

#include "world.hpp"
#include "geometry.hpp"
#include <glm/gtx/string_cast.hpp>
#include "time.hpp"
#include "filelocator.hpp"
#ifdef __APPLE__
#include <unistd.h>
#endif
#include <filesystem>
#include <algorithm>
#if defined _WIN32 || defined _WIN64
#include <windows.h>
#endif

World World::instance;

// template class LongConcurrentMap<Chunk*>;


#if 0
// XXX move into file services class, which includes code to compute filenames
bool file_exists(const std::string& fname)
{
    struct stat buffer;   
    return (stat(fname.c_str(), &buffer) == 0); 
}
#endif


World::~World()
{
    stopLoadSaveThread();
    stopTickThread();
}


void World::listAllChunks(std::vector<Chunk *>& list)
{
    list.clear();
    std::unique_lock<spinlock> lock(World::instance.storage_mutex);
    for (auto i=World::instance.chunk_storage.begin(); i!=World::instance.chunk_storage.end(); ++i) {
        list.push_back(i->second);
    }
}

void World::listAllEntities(std::vector<std::shared_ptr<Entity>>& list)
{
    std::unique_lock<spinlock> lock(World::instance.storage_mutex);
    list.assign(entities.begin(), entities.end());
}

// Assumes lock is already taken
Chunk* World::loadChunkUnlocked(const ChunkPos& pos)
{
    // XXX chunk gen should be done without lock being held.
    // Only hold lock when modifying chunk_storage
    
    // XXX check unload queue for chunk
    
    Chunk *chunk = 0;
    
    // Search unload queue for this chunk
    for (auto i=chunk_unload_queue.begin(); i!=chunk_unload_queue.end(); ++i) {
        Chunk *c = *i;
        if (c->getChunkPos() == pos) {
            chunk = c;
            chunk_unload_queue.erase(i);
            break;
        }
    }
    
    if (!chunk) {
        chunk = new Chunk(pos);
        bool ok = chunk->load();
        // if (!ok) do world gen
        chunk->updateAllBlocks();
        
        // XXX is this the best place to do this?
        for (int n=0; n<facing::NUM_FACES; n++) {
            ChunkPos cp = pos.neighbor(n);
            Chunk *chunk = getChunkUnlocked(cp, World::NoLoad);
            if (chunk) chunk->updateAllBlocks();
        }
    }
    
    // if (!chunk) {
    //     chunk = new Chunk(pos);
    // }

    uint64_t packed = pos.packed();
    chunk_storage[packed] = chunk;

    // last_chunk_pos = packed;
    // last_chunk = chunk;
    
    last_chunk_pos[1] = last_chunk_pos[0];
    last_chunk[1] = last_chunk[0];
    last_chunk_pos[0] = packed;
    last_chunk[0] = chunk;
    
    return chunk;
}

void World::doBlockUpdates()
{
    std::vector<BlockPos> load_pos, no_load_pos;
    load_pos.insert(load_pos.begin(), block_update_queue_load.begin(), block_update_queue_load.end());
    no_load_pos.insert(no_load_pos.begin(), block_update_queue_no_load.begin(), block_update_queue_no_load.end());
    block_update_queue_load.clear();
    block_update_queue_no_load.clear();
    
    std::unique_ptr<Chunk* []> load_chunks(new Chunk *[load_pos.size()]);
    std::unique_ptr<Chunk* []> no_load_chunks(new Chunk *[no_load_pos.size()]);
    getChunks(load_pos, load_chunks.get(), false);
    getChunks(no_load_pos, no_load_chunks.get(), true);
    
    for (int i=0; i<load_pos.size(); i++) {
        Chunk *p = load_chunks[i];
        if (p) p->updateBlock(load_pos[i]);
    }
    
    for (int i=0; i<no_load_pos.size(); i++) {
        Chunk *p = no_load_chunks[i];
        if (p) p->updateBlock(no_load_pos[i]);
    }
}

void World::updateSurroundingBlocks(const BlockPos& pos, bool no_load)
{
    BlockPos block_pos[10];
    pos.allSurrounding(block_pos);
    block_pos[8] = pos.up();
    block_pos[9] = pos.down();
    for (int neigh=0; neigh<10; neigh++) {
        updateBlock(block_pos[neigh]);
    }    
}

#if 0
void World::setBlock(const BlockPos& pos, const std::string& block_name, int rotation)
{
    // Compute all neighbor block posions
    BlockPos block_pos[11];
    pos.allSurrounding(block_pos);
    block_pos[8] = pos;
    block_pos[9] = pos.up();
    block_pos[10] = pos.down();
    
    // Look up all corresponding chunks
    Chunk *chunks[11];
    getChunks(block_pos, 11, chunks);
    
    // Change selected block
    chunks[8]->setBlock(pos, block_name, rotation);
    
    // Mark neighbors for update
    // for (int neigh=0; neigh<11; neigh++) {
    //     chunks[neigh]->updateBlock(block_pos[neigh]);
    // }
}
#endif

void World::setBlock(const BlockPos& pos, const std::string& block_name, int rotation)
{
    Chunk *chunk = getChunk(pos.getChunkPos());
    chunk->setBlock(pos, block_name, rotation);
    updateSurroundingBlocks(pos, false);
}

void World::breakBlock(const BlockPos& pos)
{
    setBlock(pos, "air");
}

void World::queueBlock(const BlockPos& pos, const std::string& block_name)
{
    block_queue_pos.push_back(pos);
    block_queue_name.push_back(block_name);
    
    updateSurroundingBlocks(pos);
    
    if (block_queue_pos.size() > 128) flushBlockQueue();
}

void World::flushBlockQueue()
{
    //Chunk *chunks[block_queue_pos.size()];
    std::unique_ptr<Chunk* []> chunks(new Chunk *[block_queue_pos.size()]);
    getChunks(block_queue_pos, chunks.get());
        
    for (int i=0; i<block_queue_pos.size(); i++) {
        Chunk *chunk = chunks[i];
        const BlockPos& pos(block_queue_pos[i]);
        const std::string& name(block_queue_name[i]);
        // if (name == "[update]") {
        //     chunk->updateBlock(pos);
        // } else {
        //     chunk->setBlock(pos, name);
        // }
        chunk->setBlock(pos, name);
    }
    
    doBlockUpdates();    
    
    block_queue_pos.clear();
    block_queue_name.clear();
}

void World::getChunks(const ChunkPos *pos, int num_pos, Chunk **chunks, bool no_load)
{
    std::unique_lock<spinlock> lock(storage_mutex);
    for (int i=0; i<num_pos; i++) {
        chunks[i] = getChunkUnlocked(pos[i], no_load);
    }
}

void World::getChunks(const BlockPos *pos, int num_pos, Chunk **chunks, bool no_load)
{
    std::unique_lock<spinlock> lock(storage_mutex);
    for (int i=0; i<num_pos; i++) {
        chunks[i] = getChunkUnlocked(pos[i].getChunkPos(), no_load);
    }
}

void World::getBlocks(const BlockPos *block_pos, int num_pos, BlockPtr *blocks, bool no_load)
{
    //Chunk *chunks[num_pos];
    std::unique_ptr<Chunk*[]> chunks(new Chunk *[num_pos]);
    getChunks(block_pos, num_pos, chunks.get(), no_load);
    for (int i=0; i<num_pos; i++) {
        if (chunks[i]) {
            blocks[i] = chunks[i]->getBlock(block_pos[i]);
        } else {
            blocks[i] = 0;
        }
    }
}

void World::getNeighborBlocks(const BlockPos& pos, BlockPtr *blocks, bool include_self, bool no_load)
{
    BlockPos block_pos[facing::NUM_FACES+1];
    pos.allNeighbors(block_pos);
    block_pos[facing::NUM_FACES] = pos;
    getBlocks(block_pos, facing::NUM_FACES + (include_self?1:0), blocks, no_load);
}

void World::getSurroundingBlocks(const BlockPos& pos, BlockPtr *blocks, bool include_self, bool no_load)
{
    BlockPos block_pos[9];
    pos.allSurrounding(block_pos);
    block_pos[8] = pos;
    getBlocks(block_pos, 8 + (include_self?1:0), blocks, no_load);
}


void World::findNearestBlock(const glm::dvec3& start, const glm::dvec3& forward, double limit, BlockPos& target, double& dist, int& enter_face)
{
    double r;
    dist = 0;
    enter_face = -1;
    glm::dvec3 here = start;
    for (int i=0; i<100; i++) {
        int exit_face = geom::exitFace(here, forward, r);
        // std::cout << "Exit: here=" << glm::to_string(here) << " forward=" << glm::to_string(forward) << " r=" << r << " dist=" << dist << " limit=" << limit << " face=" << facing::faceName(exit_face) << std::endl;
        if (exit_face<0 || r+dist>limit) { 
            // std::cout << "Exiting\n";
            enter_face=-1; dist=-1; return; 
        }
        // if (r<0.0001) r = 1;
        
        dist += r;
        enter_face = facing::oppositeFace(exit_face);
        here = start + forward * dist;
        target = geom::whichBlock(here, forward);
        
        BlockPtr block = World::instance.getBlock(target, World::NoLoad);
        // std::cout << "Block type " << block << " at " << target.toString() << " = " << BlockLibrary::instance.getBlockName(block) << std::endl;
        if (block) return;
    }
    exit(0);
}

void World::allIntersectingCollisions(std::vector<geom::Box>& collisions, const geom::Box& focus)
{
    BlockPosArray focus_block_pos;
    focus.intBoxes(focus_block_pos);
    size_t nblocks = focus_block_pos.arr.size();
    //BlockPtr block_ptrs[nblocks];
    std::unique_ptr<BlockPtr[]> block_ptrs(new BlockPtr[nblocks]);
    World::instance.getBlocks(focus_block_pos.arr, block_ptrs.get());
    for (int i=0; i<nblocks; i++) {
        if (!block_ptrs[i] || block_ptrs[i]->isAir()) continue;
        std::vector<geom::Box> block_collisions;
        block_ptrs[i]->getCollision(block_collisions);
        for (auto j=block_collisions.begin(); j!=block_collisions.end(); ++j) {
            if (focus.intersects(*j)) {
                collisions.push_back(*j);
            }
        }
    }
}


Chunk* World::nextSaveChunk()
{
    // Find a modified chunk saved more than 5 seconds in the past
    Chunk *oldest = 0;
    {
        double min_time = 0;
        std::unique_lock<spinlock> lock(storage_mutex);
        for (auto i=chunk_storage.begin(); i!=chunk_storage.end(); ++i) {
            Chunk *chunk = i->second;
            if ((!oldest || chunk->last_save < min_time) && chunk->needs_save) {
                min_time = chunk->last_save;
                oldest = chunk;
            }
        }
    }
    
    if (!oldest) return 0;
    double now = ref::currentTime();
    double age = now - oldest->last_save;
    if (age < 5) return 0;
    return oldest;
}

void World::saveChunk(Chunk* chunk)
{
    chunk->save();
}

void World::saveAll()
{
    std::unique_lock<spinlock> lock(storage_mutex);
    for (auto i=chunk_storage.begin(); i!=chunk_storage.end(); ++i) {
        Chunk *chunk = i->second;
        saveChunk(chunk);
    }
}

void World::unloadChunk(Chunk* chunk)
{
    // Put the chunk into the unload queue
    std::unique_lock<spinlock> lock(storage_mutex);
    chunk->time_unloaded = ref::currentTime();
    uint64_t packed = chunk->getChunkPos().packed();
    chunk_storage.erase(packed);
    chunk_unload_queue.push_back(chunk);
    
    last_chunk[1] = 0;
    last_chunk[0] = 0;
}

void World::dequeueUnloadedChunk()
{
    std::unique_lock<spinlock> lock(storage_mutex);
    // Save the oldest chunk and remove from the queue
    if (chunk_unload_queue.size() < 1) return;
    auto i = chunk_unload_queue.begin();
    Chunk *chunk = *i;
    double now = ref::currentTime();
    double age = now - chunk->time_unloaded;
    if (age < 1) return;
    saveChunk(chunk);
    chunk_unload_queue.erase(i);
}

void World::useAction(const BlockPos& pos, int face)
{
    BlockPtr block = getBlock(pos);
    bool consumed = block->useAction(face);
    if (consumed) return;
    if (block_to_place.size() > 0) {
        setBlock(pos.neighbor(face), block_to_place, place_rotation);
    }
}

void World::hitAction(const BlockPos& pos, int face)
{
    BlockPtr block = getBlock(pos);
    bool consumed = block->hitAction(face);
    if (consumed) return;
    breakBlock(pos);
}






void World::stopLoadSaveThread()
{
    if (loadSaveThread) {
        ls_thread_alive = false;
        loadSaveThread->join();
        delete loadSaveThread;
        loadSaveThread = 0;
    }
}

void World::startLoadSaveThread()
{
    ls_thread_alive = true;
    loadSaveThread = new std::thread(&World::loadSaveThreadLoop, this);
}



void World::loadSomeChunk(const BlockPos& center)
{
    std::unique_lock<spinlock> lock(storage_mutex);
    
    if (load_chunk_index<0 || load_chunk_index>=known_chunks.size()) {
        load_chunk_index = 0;
        sortKnownChunks(center);
    }
    
    while (load_chunk_index < known_chunks.size()) {
        const ChunkPos& cp(known_chunks[load_chunk_index++]);
        if (!chunkIsLoadedUnlocked(cp)) {
            // Check distance
            loadChunkUnlocked(cp);
            return;
        }
    }
}

void World::loadSaveThreadLoop()
{
    loadKnownChunks();
    while (ls_thread_alive) {
        // std::cout << "loadSaveThreadLoop\n";
#ifdef __APPLE__
        usleep(10000);
#endif
#if defined _WIN32 || defined _WIN64
        Sleep(10);
#endif
        Chunk *chunk = nextSaveChunk();
        if (chunk) {
            saveChunk(chunk);
        }
        
        loadSomeChunk(BlockPos(0,0,0));
        
        // unloadSomeChunk();
    }
}

void World::addKnownChunk(const ChunkPos& pos)
{
    for (auto i=known_chunks.begin(); i!=known_chunks.end(); i++) {
        if (*i == pos) return;
    }
    known_chunks.push_back(pos);
}

template<typename T>
inline T sqr(T x) { return x*x; }

void World::sortKnownChunks(const BlockPos& center)
{
    ChunkPos cp(center.getChunkPos());
    
    std::sort(known_chunks.begin(), known_chunks.end(),
    [&cp](const ChunkPos& a, const ChunkPos& b) {
        long dist_a = sqr(a.X-cp.X) + sqr(a.Y-cp.Y) + sqr(a.Z-cp.Z);
        long dist_b = sqr(b.X-cp.X) + sqr(b.Y-cp.Y) + sqr(b.Z-cp.Z);
        return dist_a < dist_b;
    });
}

static bool parse_chunk_coords(const char *str, int *arr)
{
    while (*str && *str!='(') str++;
    if (*str != '(') return false;
    str++;
    arr[0] = atoi(str);

    while (*str && *str!=',') str++;
    if (*str != ',') return false;
    str++;
    arr[1] = atoi(str);
    
    while (*str && *str!=',') str++;
    if (*str != ',') return false;
    str++;
    arr[2] = atoi(str);
    
    return true;
}

void World::loadKnownChunks()
{
    bool ok;
    
    known_chunks.clear();
    
    std::string path = FileLocator::instance.chunk("");
    for (const auto& entry : std::filesystem::directory_iterator(path)) {
        const std::string& path(entry.path().string());
        std::size_t found = path.find_last_of("/\\");
        if (found == std::string::npos) continue;
        const char *fname = path.c_str() + found + 1;
        int xyz[3];
        ok = parse_chunk_coords(fname, xyz);
        if (!ok) continue;
        
        known_chunks.emplace_back(xyz[0], xyz[1], xyz[2]);
        // std::cout << fname << " -> " << known_chunks.back().toString() << std::endl;
    }
}


void World::startTickThread()
{
    tick_thread_alive = true;
    tickThread = new std::thread(&World::tickThreadLoop, this);    
}

void World::stopTickThread()
{
    if (tickThread) {
        tick_thread_alive = false;
        tickThread->join();
        delete tickThread;
        tickThread = 0;
    }
}

void World::tickThreadLoop()
{
    last_tick_time = ref::currentTime();
    while (tick_thread_alive) {        
        double before_ticking = ref::currentTime();
        tickEverything(before_ticking - last_tick_time);
        last_tick_time = before_ticking;
        double after_ticking = ref::currentTime();
        
        double elapsed = after_ticking - before_ticking;
        double sleep_needed = 0.05 - elapsed;
        if (sleep_needed > 0) {
#ifdef __APPLE__
            usleep((int)(sleep_needed * 1000000.0));
#endif
#if defined _WIN32 || defined _WIN64
            Sleep((int)(sleep_needed * 1000.0));
#endif
        }
    }
}

void World::tickEverything(double elapsed_time)
{
    std::vector<Chunk *> chunks;
    std::vector<std::shared_ptr<Entity>> entities;
    
    World::instance.listAllChunks(chunks);
    
    for (auto i=chunks.begin(); i!=chunks.end(); ++i) {
        (*i)->tickAllBlocks(elapsed_time);
    }

    World::instance.listAllEntities(entities);    
    
    for (auto i=entities.begin(); i!=entities.end(); ++i) {
        (*i)->gameTick(elapsed_time);
    }
}
