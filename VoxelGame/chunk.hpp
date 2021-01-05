#ifndef INCLUDED_CHUNK_HPP
#define INCLUDED_CHUNK_HPP

#include "stdint.h"
#include "constants.hpp"
#include <unordered_map>
#include "position.hpp"
// #include "chunkview.hpp"
#include "datacontainer.hpp"
#include "mesh.hpp"
#include "blocktype.hpp"

class ChunkView;

struct Block;
typedef std::shared_ptr<Block> BlockPtr;

class Chunk {
    friend class ChunkView;
    
public:
    double last_save, time_unloaded;
    bool needs_save;
    
private:
    ChunkPos chunk_pos;
    std::unique_ptr<ChunkView> view;
    
    // Chunk storage
    uint16_t block_storage[sizes::chunk_storage_size];
    uint8_t block_rotation[sizes::chunk_storage_size];
    std::unordered_map<uint16_t, DataContainerPtr> data_containers;    
    MeshPtr meshes[sizes::chunk_storage_size];
    
    
    // Library of block IDs
    std::vector<BlockType *> index2name;
    std::unordered_map<std::string, uint16_t> name2index;
    
    BlockType *lookupBlockType(uint16_t id) {
        // if (id >= index2name.size()) return 0;
        return index2name[id];
    }
    
    uint16_t getBlockID(const std::string& name);
    uint16_t getBlockID(BlockType *bt);

public:
    static uint16_t chunkBlockIndex(const BlockPos& pos) {
        uint16_t x = pos.X & 15;
        uint16_t y = pos.Y & 15;
        uint16_t z = pos.Z & 15;
        return x | (z<<4) | (y<<8);
    }
    
    static void decodeIndex(uint32_t index, int& x, int& y, int& z) {
        x = index & 15;
        z = (index >> 4) & 15;
        y = (index >> 8) & 15;
    }
    
    BlockPos decodeIndex(uint32_t index) {
        BlockPos bpos;
        int x, y, z;
        decodeIndex(index, x, y, z);
        bpos.X = (chunk_pos.X<<4) | x;
        bpos.Y = (chunk_pos.Y<<4) | y;
        bpos.Z = (chunk_pos.Z<<4) | z;
        return bpos;
    }
    
public:
    Chunk(ChunkPos p);
    ~Chunk();
    
    BlockPtr getBlock(const BlockPos& pos);
    BlockPtr getBlock(uint16_t index);
    void breakBlock(const BlockPos& pos);
    void setBlock(const BlockPos& pos, const std::string& name, int rotation);
    void setBlock(const BlockPos& pos, const std::string& name) {
        setBlock(pos, name, 0);
    }
    void updateBlock(const BlockPos& pos); // causes visual update and update event
    void updateAllBlocks(); // Visual and update event to all blocks
    
    // Used by Block, not externally
    void requestVisualUpdate(Block *block); // This requests ONLY visual update, after Block has done some work
    int getVisibleFaces(Block *block);
    void setVisibleFaces(Block *block, int faces);
    int getRotation(Block *block);
    void setRotation(Block *block, int rot);
    int getRotation(uint16_t index) {
        return block_rotation[index];
    }
    
    MeshPtr getMesh(uint16_t index) {
        MeshPtr mp = meshes[index];
        if (mp) return mp;
        BlockType *bt = lookupBlockType(block_storage[index]);
        return bt->getMesh();
    }
    MeshPtr getMesh(Block *block);
    void setMesh(Block *block, MeshPtr mesh);
    
    void setRepeatTickFrequency(Block *block, int freq) { }
    void scheduleFutureTick(Block *block, int ticks_later) { }
    
    void tickAllBlocks(double elapsed_time);
    
    // Call after modifying data
    void markDataModified() { needs_save = true; }
    DataContainerPtr getDataContainer(Block *block, bool create);
    
    
    void getCorners(BlockPos *pos, const BlockPos& center);
    
    void save();
    bool load();
    
    const ChunkPos& getChunkPos() { return chunk_pos; }
    
    ChunkView* getView();
};

#endif
