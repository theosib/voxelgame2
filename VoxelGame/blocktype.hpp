#ifndef INCLUDED_BLOCK_TYPE_HPP
#define INCLUDED_BLOCK_TYPE_HPP

#include "mesh.hpp"

struct Block;

// Common base class for all types of blocks. These implement functionality, but data is stored in 
// Block objects.
class BlockType {
public:
    
    static const bool DefaultAction = false;
    
    virtual ~BlockType();
    
    // Get default mesh
    virtual MeshPtr getMesh() = 0;
    
    virtual bool hitAction(Block *block, int face) = 0;
    virtual bool useAction(Block *block, int face) = 0;
    virtual void tickEvent(Block *block, int tick_types) = 0;
    virtual void placeEvent(Block *block) = 0;
    virtual void breakEvent(Block *block) = 0;
    virtual void updateEvent(Block *block) = 0;
    
    virtual const std::string& getName() = 0;
};

#endif
