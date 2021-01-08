#ifndef INCLUDED_BLOCK_HPP
#define INCLUDED_BLOCK_HPP

/*
Tick types:
- repeat (regular interval)
- scheduled (single event specified in the past)
*/

#include "mesh.hpp"
#include "datacontainer.hpp"
#include "chunk.hpp"

class BlockType;
struct Block;
typedef std::shared_ptr<Block> BlockPtr;

// Descriptor for block instances
// Created temporarily by Chunk::getBlock
// Forwards actions and requests to BlockType and Chunk
struct Block {
    /*** Metadata about block ***/
    
    Chunk *chunk;               // Which mesh this belongs to
    BlockPos pos;               // Coordinates of this block
    int storage_index;          // Index of this block in chunk
    
    // Implementation for this type of block
    BlockType *impl;
    BlockType *getBlockType() { return impl; }
    
    /*** Visual ***/
    
    int getVisibleFaces() { return chunk->getVisibleFaces(this); }
    void setVisibleFaces(int faces) { chunk->setVisibleFaces(this, faces); }   // XXX automatically request visual update
    
    int getRotation() { return chunk->getRotation(this); }
    void setRotation(int rot) { chunk->setRotation(this, rot); }  // XXX automatically request visual update
    
    // Gets mesh stored in chunk or static mesh from block implementation
    // If chunk doesn't have mesh, it automatically gets it from impl
    // Render thread gets mesh directly from chunk
    MeshPtr getMesh() { return chunk->getMesh(this); }
    
    // Stores a mesh into the chunk, replacing previous or default
    // XXX automatically request visual update
    void setMesh(MeshPtr mesh) { chunk->setMesh(this, mesh);  }
    
    void getCollision(std::vector<geom::Box>& collision) {
        // collision.push_back(geom::Box(pos.X, pos.Y, pos.Z, pos.X+1, pos.Y+1, pos.Z+1));
        MeshPtr mesh = getMesh();
        int rot = getRotation();
        mesh->getCollision(collision, pos.X, pos.Y, pos.Z, rot);
    }
    
    
    // Requests that this block get redrawn due to visual change (not usually necessary)
    void requestVisualUpdate() { chunk->requestVisualUpdate(this); }
    
    
    /*** Actions ***/
    
    bool hitAction(int face) { return impl->hitAction(this, face); }
    bool useAction(int face) { return impl->useAction(this, face); }
    void tickEvent(int tick_types) { impl->tickEvent(this, tick_types); }
    void updateEvent() { impl->updateEvent(this); }
    void placeEvent() { impl->placeEvent(this); }
    void breakEvent() { impl->breakEvent(this); }
    
    void setRepeatTickFrequency(int freq) { chunk->setRepeatTickFrequency(this, freq); }
    void scheduleFutureTick(int ticks_later) { chunk->scheduleFutureTick(this, ticks_later); }
    
    // Get data container from chunk
    // If container doesn't exist and create is true, then one will be created
    DataContainerPtr getData(bool create) { return chunk->getDataContainer(this, create); }
    
    // Turn block into data. This is not for disk storage but for moving blocks around.
    // Caller must clear deque
    // void serialize(deque<char> bytes);
    
    static BlockPtr makeBlock() { return std::shared_ptr<Block>(new Block); }
    
    bool isAir() { return impl == 0; }
};


/*
NOTES:
- Chunk break block must clear mesh and data and scheduled ticks and request visual update
*/

#endif