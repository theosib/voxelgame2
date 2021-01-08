#include "blocktype.hpp"
#include "blocklibrary.hpp"
#include "block.hpp"
#include "world.hpp"

class DynamicDirtBlock : public BlockType {
private:
    MeshPtr default_mesh;
    std::string name;
    MeshPtr using_mesh;
    float saved_height[9];
    
public:
    DynamicDirtBlock();
    virtual ~DynamicDirtBlock();
    
    // Get default mesh
    virtual MeshPtr getMesh();
    
    virtual bool hitAction(Block *block, int face);
    virtual bool useAction(Block *block, int face);
    virtual void tickEvent(Block *block, int tick_types);
    virtual void placeEvent(Block *block);
    virtual void breakEvent(Block *block);
    virtual void updateEvent(Block *block);

    virtual const std::string& getName();
    
    void clear_height() {
        memset(saved_height, 0, sizeof(saved_height));
    }
};

DynamicDirtBlock::DynamicDirtBlock()
{
    default_mesh = Mesh::makeMesh();
    name = "dirt";
    default_mesh->loadMesh(name);
    clear_height();
}

DynamicDirtBlock::~DynamicDirtBlock() {}

MeshPtr DynamicDirtBlock::getMesh()
{
    return default_mesh;
}

bool DynamicDirtBlock::hitAction(Block *block, int face) { return BlockType::DefaultAction; }
bool DynamicDirtBlock::useAction(Block *block, int face) { return BlockType::DefaultAction; }
void DynamicDirtBlock::tickEvent(Block *block, int tick_types) {}

void DynamicDirtBlock::placeEvent(Block *block) 
{
    updateEvent(block);
}

void DynamicDirtBlock::breakEvent(Block *block) {}

static bool heights_equal(float *p, float *q)
{
    for (int i=0; i<9; i++) {
        if (p[i] != q[i]) return false;
    }
    return true;
}

void DynamicDirtBlock::updateEvent(Block *block)
{
    // block->setMesh(default_mesh);
    // std::cout << "Update\n";
    
#if 0
    BlockPos npos[8];
    npos[0] = block->pos.north().west();
    npos[1] = block->pos.north();
    npos[2] = block->pos.north().east();
    npos[3] = block->pos.west();
    npos[4] = block->pos.east();
    npos[5] = block->pos.south().west();
    npos[6] = block->pos.south();
    npos[7] = block->pos.south().east();
    BlockPtr neigh[8];
    World::instance.getBlocks(npos, 8, neigh, World::NoLoad);
#endif
    
    BlockPtr neigh[8];
    World::instance.getSurroundingBlocks(block->pos, neigh, false, World::NoLoad);
            
    if (neigh[1] && neigh[3] && neigh[4] && neigh[6]) {
        if (using_mesh) {
            // Surrounded on all sides: use default
            block->setMesh(0);
            using_mesh.reset();
            World::instance.updateSurroundingBlocks(block->pos);
        }
        return;
    }
        
    for (int i=0; i<8; i++) {
        if (neigh[i] && neigh[i]->getBlockType() == this) neigh[i].reset();
    }
    
    float height[9];
    for (int i=0; i<8; i++) height[i] = 0.0625;
    if (neigh[0]) height[0] = 1;
    if (neigh[2]) height[2] = 1;
    if (neigh[1]) height[0] = height[2] = 1;
    // if (neigh[1]) height[1] = 1;
    if (neigh[5]) height[5] = 1;
    if (neigh[3]) height[5] = height[0] = 1;
    // if (neigh[3]) height[3] = 1;
    if (neigh[7]) height[7] = 1;
    if (neigh[6]) height[7] = height[5] = 1;
    // if (neigh[6]) height[6] = 1;
    if (neigh[4]) height[2] = height[7] = 1;
    // if (neigh[4]) height[4] = 1;

    height[1] = (height[0] + height[2]) * 0.5f;
    height[3] = (height[0] + height[5]) * 0.5f;
    height[6] = (height[5] + height[7]) * 0.5f;
    height[4] = (height[2] + height[7]) * 0.5f;
    // height[8] = (height[0] + height[2] + height[5] + height[7]) * 0.25;
    height[8] = 0;
    for (int i=0; i<8; i++) height[8] += height[i];
    height[8] *= 0.125;
    if (height[8] < 0.5) height[8] = 0.5;
    
    if (using_mesh && heights_equal(height, saved_height)) return;
            
    // Build new mesh
    MeshPtr newMesh = Mesh::makeMesh();
    newMesh->setTexture(default_mesh->getTextureIndex());
    newMesh->setSolidFaces(facing::DOWN_MASK);
    *(newMesh->getFace(facing::DOWN)) = *(default_mesh->getFace(facing::DOWN));
    
    // for (int i=0; i<9; i++) height[i] = 0.5;
    // for (int i=0; i<8; i++) height[i] = 0.8;
    
    
    Face *f;
    
    f = newMesh->addFace();
    f->addVertex(1, height[2], 0);
    f->addVertex(1, 0, 0);
    f->addVertex(0.5, 0, 0);
    f->addVertex(0.5, height[1], 0);
    f->addTexCoord(0, height[2]);
    f->addTexCoord(0, 0);
    f->addTexCoord(0.5, 0);
    f->addTexCoord(0.5, height[1]); // check

    f = newMesh->addFace();
    f->addVertex(0.5, height[1], 0);
    f->addVertex(0.5, 0, 0);
    f->addVertex(0, 0, 0);
    f->addVertex(0, height[0], 0);
    f->addTexCoord(0.5, height[1]);
    f->addTexCoord(0.5, 0);
    f->addTexCoord(1, 0);
    f->addTexCoord(1, height[0]); // check

    f = newMesh->addFace();
    f->addVertex(0, height[0], 0);
    f->addVertex(0, 0, 0);
    f->addVertex(0, 0, 0.5);
    f->addVertex(0, height[3], 0.5);
    f->addTexCoord(0, height[0]);
    f->addTexCoord(0, 0);
    f->addTexCoord(0.5, 0);
    f->addTexCoord(0.5, height[3]); // check

    f = newMesh->addFace();
    f->addVertex(0, height[3], 0.5);
    f->addVertex(0, 0, 0.5);
    f->addVertex(0, 0, 1);
    f->addVertex(0, height[5], 1);
    f->addTexCoord(0.5, height[3]);
    f->addTexCoord(0.5, 0);
    f->addTexCoord(1, 0);
    f->addTexCoord(1, height[5]); // check
    
    f = newMesh->addFace();
    f->addVertex(0, height[5], 1);
    f->addVertex(0, 0, 1);
    f->addVertex(0.5, 0, 1);
    f->addVertex(0.5, height[6], 1);
    f->addTexCoord(0, height[5]);
    f->addTexCoord(0, 0);
    f->addTexCoord(0.5, 0);
    f->addTexCoord(0.5, height[6]); // check

    f = newMesh->addFace();
    f->addVertex(0.5, height[6], 1);
    f->addVertex(0.5, 0, 1);
    f->addVertex(1, 0, 1);
    f->addVertex(1, height[7], 1);
    f->addTexCoord(0.5, height[6]);
    f->addTexCoord(0.5, 0);
    f->addTexCoord(1, 0);
    f->addTexCoord(1, height[7]); // check
    
    f = newMesh->addFace();
    f->addVertex(1, height[7], 1);
    f->addVertex(1, 0, 1);
    f->addVertex(1, 0, 0.5);
    f->addVertex(1, height[4], 0.5);
    f->addTexCoord(0, height[7]);
    f->addTexCoord(0, 0);
    f->addTexCoord(0.5, 0);
    f->addTexCoord(0.5, height[4]);

    f = newMesh->addFace();
    f->addVertex(1, height[4], 0.5);
    f->addVertex(1, 0, 0.5);
    f->addVertex(1, 0, 0);
    f->addVertex(1, height[2], 0);
    f->addTexCoord(0.5, height[4]);
    f->addTexCoord(0.5, 0);
    f->addTexCoord(1, 0);
    f->addTexCoord(1, height[2]);
    
    f = newMesh->addFace();
    f->addVertex(0, height[0], 0);
    f->addVertex(0, height[3], 0.5);
    f->addVertex(0.5, height[8], 0.5);
    f->addTexCoord(0, 1);
    f->addTexCoord(0, 0.5);
    f->addTexCoord(0.5, 0.5);
    
    f = newMesh->addFace();
    f->addVertex(0, height[0], 0);
    f->addVertex(0.5, height[8], 0.5);
    f->addVertex(0.5, height[1], 0);
    f->addTexCoord(0, 1);
    f->addTexCoord(0.5, 0.5);
    f->addTexCoord(0.5, 1);
    
    f = newMesh->addFace();
    f->addVertex(0.5, height[1], 0);
    f->addVertex(0.5, height[8], 0.5);
    f->addVertex(1, height[2], 0);
    f->addTexCoord(0.5, 1);
    f->addTexCoord(0.5, 0.5);
    f->addTexCoord(1, 1);
    
    f = newMesh->addFace();
    f->addVertex(1, height[2], 0);
    f->addVertex(0.5, height[8], 0.5);
    f->addVertex(1, height[4], 0.5);
    f->addTexCoord(1, 1);
    f->addTexCoord(0.5, 0.5);
    f->addTexCoord(1, 0.5);
    
    f = newMesh->addFace();
    f->addVertex(0, height[3], 0.5);
    f->addVertex(0, height[5], 1);
    f->addVertex(0.5, height[8], 0.5);
    f->addTexCoord(0, 0.5);
    f->addTexCoord(0, 0);
    f->addTexCoord(0.5, 0.5);
    
    f = newMesh->addFace();
    f->addVertex(0.5, height[8], 0.5);
    f->addVertex(0, height[5], 1);
    f->addVertex(0.5, height[6], 1);
    f->addTexCoord(0.5, 0.5);
    f->addTexCoord(0, 0);
    f->addTexCoord(0.5, 0);
    
    f = newMesh->addFace();
    f->addVertex(0.5, height[8], 0.5);
    f->addVertex(0.5, height[6], 1);
    f->addVertex(1, height[7], 1);
    f->addTexCoord(0.5, 0.5);
    f->addTexCoord(0.5, 0);
    f->addTexCoord(1, 0);
    
    f = newMesh->addFace();
    f->addVertex(1, height[4], 0.5);
    f->addVertex(0.5, height[8], 0.5);
    f->addVertex(1, height[7], 1);
    f->addTexCoord(1, 0.5);
    f->addTexCoord(0.5, 0.5);
    f->addTexCoord(1, 0);
    
    block->setMesh(newMesh);
    memcpy(saved_height, height, sizeof(saved_height));
    using_mesh = newMesh;
    World::instance.updateSurroundingBlocks(block->pos);
}

const std::string& DynamicDirtBlock::getName()
{
    return name;
}

static DynamicDirtBlock *dirt_block;

void init_dirt_block()
{
    DynamicDirtBlock *ptr = new DynamicDirtBlock();
    BlockLibrary::instance.registerBlockType("dirt", ptr);
}
