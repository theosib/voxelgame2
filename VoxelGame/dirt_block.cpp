#include "blocktype.hpp"
#include "blocklibrary.hpp"
#include "block.hpp"
#include "world.hpp"

class DynamicDirtBlock : public BlockType {
private:
    MeshPtr default_mesh;
    std::string name;
    // MeshPtr using_mesh; //prev_mesh;
    // float saved_height[9];
    
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
    
    float *getHeights(Block *block);
};

DynamicDirtBlock::DynamicDirtBlock()
{
    default_mesh = Mesh::makeMesh();
    name = "dirt";
    default_mesh->loadMesh(name);
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
    std::cout << "Placing block at " << block->pos.toString() << std::endl;
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


float *DynamicDirtBlock::getHeights(Block *block)
{
    DataContainerPtr dcp = block->getData(true);
    float *saved_height = 0;
    DataItemPtr sh = dcp->getNamedItem("heights");
    if (!sh) {
        sh = DataItem::makeFloatArray(9);
        dcp->setNamedItem("heights", sh);
        saved_height = sh->getFloatArray();
        for (int i=0; i<9; i++) saved_height[i] = -1;
        block->markDataModified();
    } else {
        saved_height = sh->getFloatArray();
    }
    return saved_height;
}


void DynamicDirtBlock::updateEvent(Block *block)
{
    // block->setMesh(default_mesh);
    // std::cout << "Update\n";
        
    BlockPtr neigh[8];
    World::instance.getSurroundingBlocks(block->pos, neigh, false, World::NoLoad);
    
    
    if (neigh[0] && neigh[2] && neigh[5] && neigh[7]) {
        if (block->getMesh() != block->getDefaultMesh()) {
            // Surrounded on all sides: use default
            block->setMesh(0);
            block->setData(0);
            // prev_mesh = using_mesh;
            World::instance.updateSurroundingBlocks(block->pos);
        }
        return;
    }
    
    for (int i=0; i<8; i++) {
        if (neigh[i]) {
            BlockType *bt = neigh[i]->getBlockType();
            if (bt == this) {
                if (neigh[i]->getMesh() != neigh[i]->getDefaultMesh()) {
                    neigh[i].reset();
                    // std::cout << "Neighbor " << i << " soft dirt\n";
                } else {
                    // std::cout << "Neighbor " << i << " solid dirt\n";
                }
            } else {
                // std::cout << "Neighbor " << i << " other block\n";
            }
        }
    }
    
    float height[9];
    for (int i=0; i<8; i++) height[i] = 0;
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
    
    if (height[1]>0.1 && height[1]<0.9) height[1] = 0.6;
    if (height[3]>0.1 && height[3]<0.9) height[3] = 0.6;
    if (height[6]>0.1 && height[6]<0.9) height[6] = 0.6;
    if (height[4]>0.1 && height[4]<0.9) height[4] = 0.6;
    
    // height[8] = (height[0] + height[2] + height[5] + height[7]) * 0.25;
    height[8] = 0.6;
    int n1=0;
    if (height[0]>0.9) n1++;
    if (height[2]>0.9) n1++;
    if (height[5]>0.9) n1++;
    if (height[7]>0.9) n1++;
    if (n1==4) height[8] = 1;
    
    float *saved_height = getHeights(block);
    
    // for (int i=0; i<9; i++) {
    //     std::cout << " " << height[i];
    // }
    // std::cout << std::endl;
    // for (int i=0; i<9; i++) {
    //     std::cout << " " << saved_height[i];
    // }
    // std::cout << std::endl;
    
    if (block->getMesh() != block->getDefaultMesh() && heights_equal(height, saved_height)) {
        // std::cout << "No mesh change\n";
        return;
    }
    
            
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
    
    memcpy(saved_height, height, sizeof(float) * 9);
    block->markDataModified();
    block->setMesh(newMesh);
    // prev_mesh = using_mesh;
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
