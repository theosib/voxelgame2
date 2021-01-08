#ifndef _INCLUDED_WORLD_HPP
#define _INCLUDED_WORLD_HPP

#include <iostream>
#include <unordered_map>
#include <mutex>
#include "position.hpp"
#include "block.hpp"
#include "chunk.hpp"
#include "texture.hpp"
#include "worldview.hpp"
#include "spinlock.hpp"
#include <deque>
#include <unordered_set>

class World {
public:
    friend class WorldView;
    
    static World instance;
    
    static const bool NoLoad = true;
    
private:
    std::thread *loadSaveThread;
    bool ls_thread_alive;
    std::thread *tickThread;
    bool tick_thread_alive;
    double last_tick_time;
    
    std::string block_to_place;
    int place_rotation;
    
    std::vector<ChunkPos> known_chunks;
    int load_chunk_index;
    
    // std::mutex storage_mutex;
    std::unordered_map<uint64_t, Chunk *> chunk_storage;
    std::deque<Chunk *> chunk_unload_queue;
    spinlock storage_mutex;
    spinlock update_mutex;
    
    std::vector<BlockPos> block_queue_pos;
    std::vector<std::string> block_queue_name;
    
    std::vector<std::shared_ptr<Entity>> entities;
    
    //std::unordered_set<BlockPos> block_update_queue_load, block_update_queue_no_load;
    // std::vector<BlockPos> block_update_queue_load, block_update_queue_no_load;
    std::unordered_set<BlockPos> block_update_queue_load, block_update_queue_no_load;
        
    // XXX invalidate this on unloading
    uint64_t last_chunk_pos[2];
    Chunk *last_chunk[2];
    
    Chunk* loadChunkUnlocked(const ChunkPos& pos);
    Chunk* loadChunkLocked(const ChunkPos& pos) {
        std::unique_lock<spinlock> lock(storage_mutex);
        return loadChunkUnlocked(pos);
    }
    
    bool chunkIsLoadedUnlocked(const ChunkPos& pos) {
        return chunk_storage.find(pos.packed()) != chunk_storage.end();
    }
    bool chunkIsLoadedLocked(const ChunkPos& pos) {
        std::unique_lock<spinlock> lock(storage_mutex);
        return chunkIsLoadedUnlocked(pos);
    }
    
public:
    World() {
        last_chunk_pos[0] = 0;
        last_chunk_pos[1] = 0;
        last_chunk[0] = 0;
        last_chunk[1] = 0;
        loadSaveThread = 0;
        ls_thread_alive = false;
        load_chunk_index = -1;
        tick_thread_alive = false;
        tickThread = 0;
        place_rotation = 0;
    }
    
    ~World();
    
    void updateBlock(const BlockPos& pos, bool no_load=false) {
        std::unique_lock<spinlock> lock(update_mutex);
        if (no_load) {
            block_update_queue_no_load.insert(pos);
        } else {
            block_update_queue_load.insert(pos);
        }
    }
    void updateBlocks(const BlockPos *pos, size_t cnt, bool no_load=false);
    void updateBlocks(const std::vector<BlockPos>& pos, bool no_load=false) {
        updateBlocks(pos.data(), pos.size(), no_load);
    }
    
    
    void updateSurroundingBlocks(const BlockPos& pos, bool no_load=false);
    
    // void updateBlock(const BlockPos& pos, bool no_load) {
    //     std::unique_lock<spinlock> lock(update_queue_mutex);
    //     updateBlockUnlocked(pos, no_load);
    // }
    
    void doBlockUpdates();
            
    Chunk* getChunkUnlocked(const ChunkPos& pos, bool no_load) {
        uint64_t packed = pos.packed();

        if (packed == last_chunk_pos[0] && last_chunk[0]) return last_chunk[0];
        if (packed == last_chunk_pos[1] && last_chunk[1]) return last_chunk[1];

        auto i = chunk_storage.find(packed);
        if (i != chunk_storage.end()) {
            last_chunk_pos[1] = last_chunk_pos[0];
            last_chunk[1] = last_chunk[0];
            last_chunk_pos[0] = packed;
            last_chunk[0] = i->second;
            return last_chunk[0];
        }
        
        if (no_load) return 0;
        return loadChunkUnlocked(pos);
        /*try {
            Chunk *chunk = chunk_storage.get(packed);
            last_chunk_pos = packed;
            last_chunk = chunk;
            return chunk;
        }  catch (std::out_of_range& e) {
            // std::cout << "getChunkUnlocked Exception " << e.what() << std::endl;
            return loadChunk(pos);
        }*/
    }
    
    void listAllChunks(std::vector<Chunk *>& list);
    void listAllEntities(std::vector<std::shared_ptr<Entity>>& list);
        
    Chunk* getChunk(const ChunkPos& pos, bool no_load=false) {
        std::unique_lock<spinlock> lock(storage_mutex);
        return getChunkUnlocked(pos, no_load);
    }
    
    void getChunks(const ChunkPos *pos, int num_pos, Chunk **chunks, bool no_load=false);
    void getChunks(const std::vector<ChunkPos>& pos, Chunk **chunks, bool no_load=false)
    {
        getChunks(pos.data(), (int)pos.size(), chunks, no_load);
    }

    void getChunks(const BlockPos *pos, int num_pos, Chunk **chunks, bool no_load=false);
    void getChunks(const std::vector<BlockPos>& pos, Chunk **chunks, bool no_load=false)
    {
        getChunks(pos.data(), (int)pos.size(), chunks, no_load);
    }
    
    BlockPtr getBlock(const BlockPos& pos, bool no_load=false) {
        Chunk *chunk = getChunk(pos.getChunkPos(), no_load);
        if (!chunk) return 0;
        return chunk->getBlock(pos);
    }
    
    BlockPtr getBlockUnlocked(const BlockPos& pos, bool no_load=false) {
        Chunk *chunk = getChunkUnlocked(pos.getChunkPos(), no_load);
        if (!chunk) return 0;
        return chunk->getBlock(pos);
    }
    
    void getBlocks(const BlockPos *pos, int num_pos, BlockPtr *blocks, bool no_load=false);    
    void getBlocks(const std::vector<BlockPos>& pos, BlockPtr *blocks, bool no_load=false) {
        getBlocks(pos.data(), (int)pos.size(), blocks, no_load);
    }
    
    void getNeighborBlocks(const BlockPos& pos, BlockPtr *blocks, bool include_self, bool no_load=false);
    void getSurroundingBlocks(const BlockPos& pos, BlockPtr *blocks, bool include_self, bool no_load=false);
    
    // XXX need no-load versions of get methods


    void setBlock(const BlockPos&  pos, const std::string& block_name, int rotation);
    void setBlock(const BlockPos&  pos, const std::string& block_name) {
        setBlock(pos, block_name, 0);
    }
    void breakBlock(const BlockPos& pos);
    
    void queueBlock(const BlockPos&  pos, const std::string& block_name);
    void flushBlockQueue();
    
    void findNearestBlock(const glm::dvec3& start, const glm::dvec3& forward, double limit, BlockPos& target, double& dist, int& enter_face);
    
    void addKnownChunk(const ChunkPos& pos);
    void loadKnownChunks();
    void sortKnownChunks(const BlockPos& center);
    
    void stopLoadSaveThread();
    void startLoadSaveThread();
    void loadSaveThreadLoop();
    Chunk* nextSaveChunk();
    void saveChunk(Chunk* chunk);
    void loadSomeChunk(const BlockPos& center);
    void unloadChunk(Chunk* chunk);
    void dequeueUnloadedChunk();
    void saveAll();
    
    
    void addEntity(Entity *p) {
        entities.push_back(std::shared_ptr<Entity>(p));
    }
    
    
    void startTickThread();
    void stopTickThread();
    void tickThreadLoop();
    void tickEverything(double elapsed_time);
    
    
    void useAction(const BlockPos& pos, int face);
    void hitAction(const BlockPos& pos, int face);
    
    void incBlockRotation(int inc) {
        place_rotation += inc;
        if (place_rotation >= 24) place_rotation = 0;
        if (place_rotation < 0) place_rotation = 23;
    }
    
    void setBlockRotation(int r) {
        place_rotation = r;
    }
    
    int getBlockRotation() {
        return place_rotation;
    }
    
    void setBlockToPlace(const std::string& block) {
        block_to_place = block;
    }
    
    const std::string& getBlockToPlace() {
        return block_to_place;
    }
    
    void allIntersectingCollisions(std::vector<geom::Box>& collisions, const geom::Box& focus);
    
    // BlockPos
        
    /*
    Unload chunks by first transferring them into a queue so that any actions being done on
    a chunk in another thread can complete before unloading.
    
    Need methods to get and set blocks that do and don't cause chunk loading.
    */
};

#endif