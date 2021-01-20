

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


template<typename T>
inline T sqr(T x) { return x*x; }


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

void World::setOfLoadedChunks(std::unordered_set<ChunkPos>& set)
{
    set.clear();
    std::unique_lock<spinlock> lock(World::instance.storage_mutex);
    for (auto i=World::instance.chunk_storage.begin(); i!=World::instance.chunk_storage.end(); ++i) {
        set.insert(i->second->getChunkPos());
    }
}

void World::listAllEntities(std::vector<EntityPtr>& list)
{
    std::unique_lock<spinlock> lock(World::instance.storage_mutex);
    list.assign(entities.begin(), entities.end());
}

bool position_load = false;

// Assumes lock is already taken
Chunk* World::loadChunkUnlocked(const ChunkPos& pos)
{
    // XXX chunk gen should be done without lock being held.
    // Only hold lock when modifying chunk_storage
    
    // XXX check unload queue for chunk
    
    if (pos.Y<0) return 0;
    
    Chunk *chunk = 0;
    
    // Search unload queue for this chunk
    for (auto i=chunk_unload_queue.begin(); i!=chunk_unload_queue.end(); ++i) {
        Chunk *c = *i;
        if (c->getChunkPos() == pos) {
            
            // if (!position_load) {
            //     std::cout << "Loading chunk " << pos.toString() << " from unload queue" << std::endl;
            //     __builtin_trap();
            // }
            
            chunk = c;
            chunk->time_unloaded = 0;
            chunk_unload_queue.erase(i);
            break;
        }
    }
    
    if (!chunk) {
        // std::cout << "Loading chunk " << pos.toString() << " from disk" << std::endl;
        chunk = new Chunk(pos);
        bool ok = chunk->load();
        if (!ok) chunk->generate(); // XXX queue the generation to a gen thread
        chunk->repaintAllBlocks();
        
        // XXX is this the best place to do this?
        for (int n=0; n<facing::NUM_FACES; n++) {
            ChunkPos cp = pos.neighbor(n);
            Chunk *chunk = getChunkUnlocked(cp, World::NoLoad);
            if (chunk) chunk->repaintAllBlocks();
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

void World::updateBlocks(const BlockPos *pos, size_t count, bool no_load)
{
    std::unique_lock<spinlock> lock(update_mutex);
    if (no_load) {
        for (int i=0; i<count; i++) {
            block_update_queue_no_load.insert(pos[i]);
        }
    } else {
        for (int i=0; i<count; i++) {
            block_update_queue_load.insert(pos[i]);
        }
    }
}

void World::repaintBlocks(const BlockPos *pos, size_t count)
{
    std::unique_lock<spinlock> lock(repaint_mutex);
    for (int i=0; i<count; i++) {
        repaint_queue.insert(pos[i]);
    }
}

void World::doBlockUpdates()
{
    int start_repaint, did_repaint, after_repaint, remain_repaint;
    
    int limit = 100000;
    std::vector<BlockPos> load_pos, no_load_pos, repaint_pos;
    
    /* Get some blocks to update */
    {
        std::unique_lock<spinlock> lock(update_mutex);
        
        auto it = block_update_queue_load.begin();
        while (it!=block_update_queue_load.end() && limit>0) {
            load_pos.push_back(*it);
            //it = block_update_queue_load.erase(it);
            ++it;
            limit--;
        }
        
        it = block_update_queue_no_load.begin();
        while (it!=block_update_queue_no_load.end() && limit>0) {
            no_load_pos.push_back(*it);
            // it = block_update_queue_no_load.erase(it);
            ++it;
            limit--;
        }
        
        // load_pos.insert(load_pos.begin(), block_update_queue_load.begin(), block_update_queue_load.end());
        // no_load_pos.insert(no_load_pos.begin(), block_update_queue_no_load.begin(), block_update_queue_no_load.end());
        // block_update_queue_load.clear();
        // block_update_queue_no_load.clear();
    }
    {
        std::unique_lock<spinlock> lock(repaint_mutex);
        
        start_repaint = repaint_queue.size();
        
        auto it = repaint_queue.begin();
        while (it!=repaint_queue.end() && limit>0) {
            repaint_pos.push_back(*it);
            // it = repaint_queue.erase(it);
            ++it;
            limit--;
        }
        
        // repaint_pos.insert(repaint_pos.begin(), repaint_queue.begin(), repaint_queue.end());
        // repaint_queue.clear();
    }
    
    /* Update the blocks */
    std::unique_ptr<Chunk* []> load_chunks(new Chunk *[load_pos.size()]);
    std::unique_ptr<Chunk* []> no_load_chunks(new Chunk *[no_load_pos.size()]);
    std::unique_ptr<Chunk* []> repaint_chunks(new Chunk *[repaint_pos.size()]);
    getChunks(load_pos, load_chunks.get(), !World::NoLoad);
    getChunks(no_load_pos, no_load_chunks.get(), World::NoLoad);
    getChunks(repaint_pos, repaint_chunks.get(), World::NoLoad);
    
    // int total = load_pos.size() + no_load_pos.size();
    // if (total) std::cout << "Updating " << total << " blocks\n";
    
    for (int i=0; i<load_pos.size(); i++) {
        Chunk *p = load_chunks[i];
        if (p) p->updateBlock(load_pos[i]);
    }
    
    for (int i=0; i<no_load_pos.size(); i++) {
        Chunk *p = no_load_chunks[i];
        if (p) p->updateBlock(no_load_pos[i]);
    }
    
    for (int i=0; i<repaint_pos.size(); i++) {
        Chunk *p = repaint_chunks[i];
        if (p) p->repaintBlock(repaint_pos[i]);
    }
    
    did_repaint = repaint_pos.size();
    
    /* Remove those blocks from the queue */
    {
        std::unique_lock<spinlock> lock(update_mutex);
        
        for (auto it=load_pos.begin(); it!=load_pos.end(); ++it) {
            block_update_queue_load.erase(*it);
        }

        for (auto it=no_load_pos.begin(); it!=no_load_pos.end(); ++it) {
            block_update_queue_no_load.erase(*it);
        }
    }
    
    {
        std::unique_lock<spinlock> lock(repaint_mutex);
        
        after_repaint = repaint_queue.size();
        for (auto it=repaint_pos.begin(); it!=repaint_pos.end(); ++it) {
            repaint_queue.erase(*it);
        }
        remain_repaint = repaint_queue.size();
        
        // std::cout << "Repaints: start=" << start_repaint << " did=" << did_repaint << " after=" << after_repaint << " remain=" << remain_repaint << std::endl;
    }
}

void World::updateSurroundingBlocks(const BlockPos& pos, bool no_load)
{
    BlockPos block_pos[26];
    pos.allSurrounding(block_pos);
    updateBlocks(block_pos, 26, no_load);
}

void World::repaintSurroundingBlocks(const BlockPos& pos)
{
    BlockPos block_pos[26];
    pos.allSurrounding(block_pos);
    repaintBlocks(block_pos, 26);
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
    updateSurroundingBlocks(pos);
}

void World::breakBlock(const BlockPos& pos)
{
    setBlock(pos, "air");
}

void World::queueBlock(const BlockPos& pos, const std::string& block_name)
{
    block_queue_pos.push_back(pos);
    block_queue_name.push_back(block_name);
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
        updateSurroundingBlocks(pos);
    }
    
    // doBlockUpdates();    
    
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
    BlockPos block_pos[27];
    pos.allSurrounding(block_pos);
    if (include_self) {
        for (int i=26; i>13; i--) {
            block_pos[i] = block_pos[i-1];
        }
        block_pos[13] = pos;
        getBlocks(block_pos, 27, blocks, no_load);
    } else {
        getBlocks(block_pos, 26, blocks, no_load);
    }
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

void World::unloadChunkUnlocked(const ChunkPos& cp)
{
    //std::unique_lock<spinlock> lock(storage_mutex);
    
    Chunk *chunk = getChunkUnlocked(cp, World::NoLoad);
    if (!chunk) {
        std::cout << "Tried to unload an unloaded chunk\n";
        return;
    }
    
    // Put the chunk into the unload queue
    // std::cout << "Moving " << cp.toString() << " into unload queue\n";
    chunk->time_unloaded = ref::currentTime();
    uint64_t packed = chunk->getChunkPos().packed();
    chunk_unload_queue.push_back(chunk);
    chunk_storage.erase(packed);
    
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
    
    // std::cout << "Unloading chunk " << chunk->getChunkPos().toString() << std::endl;
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




void World::startBlockUpdateThread()
{
    bu_thread_alive = true;
    blockUpdateThread = new std::thread(&World::blockUpdateThreadLoop, this);
}

void World::stopBlockUpdateThread()
{
    if (blockUpdateThread) {
        bu_thread_alive = false;
        blockUpdateThread->join();
        delete blockUpdateThread;
        blockUpdateThread = 0;
    }
}

void World::blockUpdateThreadLoop()
{
    while (bu_thread_alive) {
        // std::cout << "loadSaveThreadLoop\n";
#ifdef __APPLE__
        usleep(10000);
#endif
#if defined _WIN32 || defined _WIN64
        Sleep(10);
#endif
        
        doBlockUpdates();
    }
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

#if 0
void World::loadSomeChunk(const BlockPos& center)
{
    std::unique_lock<spinlock> lock(storage_mutex);
    
    if (load_chunk_index<0 || load_chunk_index>=known_chunks.size()) {
        load_chunk_index = 0;
        sortKnownChunks(center);
    }
    
    const ChunkPos center_cp(center.getChunkPos());
    
    while (load_chunk_index < known_chunks.size()) {
        const ChunkPos& cp(known_chunks[load_chunk_index++]);
        
        long dist = sqr(cp.X - center_cp.X) + sqr(cp.Y - center_cp.Y) + sqr(cp.Z - center_cp.Z);
        
        if (chunkIsLoadedUnlocked(cp)) {
            if (dist < 5*5) {
                loadChunkUnlocked(cp);
                return;
            }
        } else {
            if (dist > 6*6) {
                unloadChunkUnlocked(cp);
                return;
            }
        }
    }
}
#endif


constexpr int load_distance = 5;
constexpr int max_height = 16;
static void computeAreaToLoad(const ChunkPos& center, std::unordered_set<ChunkPos>& load_area)
{
    load_area.clear();
    for (int y=0; y<max_height; y++) {
        for (int x=-load_distance; x<=load_distance; x++) {
            for (int z=-load_distance; z<=load_distance; z++) {
                load_area.insert(ChunkPos(center.X + x, y, center.Z + z));
            }
        }
    }
}

static void set_difference(const std::unordered_set<ChunkPos>& a, const std::unordered_set<ChunkPos>& b, std::unordered_set<ChunkPos>& a_minus_b)
{
    for (auto ai=a.begin(); ai!=a.end(); ++ai) {
        if (!b.count(*ai)) a_minus_b.insert(*ai);
    }
}

static void setToListAndSort(const std::unordered_set<ChunkPos>& set, std::vector<ChunkPos>& list, const ChunkPos& center)
{
    for (auto ai=set.begin(); ai!=set.end(); ++ai) {
        list.push_back(*ai);
    }
    std::sort(list.begin(), list.end(),
    [&center](const ChunkPos& a, const ChunkPos& b) {
        long dist_a = sqr(a.X-center.X) + sqr(a.Y-center.Y) + sqr(a.Z-center.Z);
        long dist_b = sqr(b.X-center.X) + sqr(b.Y-center.Y) + sqr(b.Z-center.Z);
        return dist_a < dist_b;
    });
}

void World::loadUnloadChunks(const ChunkPos& center)
{
    std::unordered_set<ChunkPos> load_area, currently_loaded, to_load_set, to_unload_set;
    
    computeAreaToLoad(center, load_area);
    setOfLoadedChunks(currently_loaded);
    
    // std::cout << "Center " << center.toString() << std::endl;
    
    set_difference(load_area, currently_loaded, to_load_set);
    if (to_load_set.size() > 0) {
        std::vector<ChunkPos> to_load_list;
        setToListAndSort(to_load_set, to_load_list, center);
        int i=0;
        for (auto li=to_load_list.begin(); i<1 && li!=to_load_list.end(); ++li, i++) {
            const ChunkPos& load_pos(*li);
            position_load = true;
            loadChunkLocked(load_pos);
            position_load = false;
        }
        // ChunkPos load_pos = *(to_load_list.begin());
        // std::cout << "Loading chunk " << load_pos.toString() << " because of position\n";
        // loadChunkLocked(load_pos);
    }
    
    set_difference(currently_loaded, load_area, to_unload_set);
    if (to_unload_set.size() > 0) {
        std::vector<ChunkPos> to_unload_list;
        setToListAndSort(to_unload_set, to_unload_list, center);
        int i=0;
        for (auto li=to_unload_list.begin(); i<1 && li!=to_unload_list.end(); ++li, i++) {
            const ChunkPos& unload_pos(*li);
            unloadChunkLocked(unload_pos);
        }
        // ChunkPos unload_pos = *(to_unload_list.begin());
        // // std::cout << "Unloading chunk " << unload_pos.toString() << " because of position\n";
        // unloadChunkLocked(unload_pos);
    }
}

void World::loadSaveThreadLoop()
{
    // loadKnownChunks();
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
        
        loadUnloadChunks(user_position.getChunkPos());
        
        dequeueUnloadedChunk();
        
        // unloadSomeChunk();
    }
}

// void World::addKnownChunk(const ChunkPos& pos)
// {
//     for (auto i=known_chunks.begin(); i!=known_chunks.end(); i++) {
//         if (*i == pos) return;
//     }
//     known_chunks.push_back(pos);
// }


// void World::sortKnownChunks(const BlockPos& center)
// {
//     ChunkPos cp(center.getChunkPos());
//
//     std::sort(known_chunks.begin(), known_chunks.end(),
//     [&cp](const ChunkPos& a, const ChunkPos& b) {
//         long dist_a = sqr(a.X-cp.X) + sqr(a.Y-cp.Y) + sqr(a.Z-cp.Z);
//         long dist_b = sqr(b.X-cp.X) + sqr(b.Y-cp.Y) + sqr(b.Z-cp.Z);
//         return dist_a < dist_b;
//     });
// }

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

// void World::loadKnownChunks()
// {
//     bool ok;
//
//     known_chunks.clear();
//
//     std::string path = FileLocator::instance.chunk("");
//     for (const auto& entry : std::filesystem::directory_iterator(path)) {
//         const std::string& path(entry.path().string());
//         std::size_t found = path.find_last_of("/\\");
//         if (found == std::string::npos) continue;
//         const char *fname = path.c_str() + found + 1;
//         int xyz[3];
//         ok = parse_chunk_coords(fname, xyz);
//         if (!ok) continue;
//
//         known_chunks.emplace_back(xyz[0], xyz[1], xyz[2]);
//         // std::cout << fname << " -> " << known_chunks.back().toString() << std::endl;
//     }
// }


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
    std::vector<EntityPtr> entities;
    
    World::instance.listAllChunks(chunks);
    
    for (auto i=chunks.begin(); i!=chunks.end(); ++i) {
        (*i)->tickAllBlocks(elapsed_time);
    }

    World::instance.listAllEntities(entities);    
    
    for (auto i=entities.begin(); i!=entities.end(); ++i) {
        (*i)->gameTick(elapsed_time);
    }
}
