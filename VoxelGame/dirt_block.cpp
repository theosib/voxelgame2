#include "blocktype.hpp"
#include "blocklibrary.hpp"
#include "block.hpp"
#include "world.hpp"
#include "spline.hpp"
#include <random>

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
    virtual void repaintEvent(Block *block);

    virtual const std::string& getName();
    
    float *getHeights(Block *block);
    
    bool isSoftDirt(BlockPtr n);
    bool isSolidBlock(BlockPtr n);
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
    repaintEvent(block);
}

void DynamicDirtBlock::breakEvent(Block *block) {}

static bool heights_equal(float *p, float *q)
{
    for (int i=0; i<17*17; i++) {
        if (round(p[i]*128) != round(q[i]*128)) return false;
    }
    return true;
}


float *DynamicDirtBlock::getHeights(Block *block)
{
    DataContainerPtr dcp = block->getData(true);
    float *saved_height = 0;
    DataItemPtr sh = dcp->getNamedItem("heights");
    if (!sh || sh->getArrayCount() != 17*17) {
        sh = DataItem::makeFloatArray(17*17);
        dcp->setNamedItem("heights", sh);
        saved_height = sh->getFloatArray();
        for (int i=0; i<17*17; i++) saved_height[i] = -1;
        block->markDataModified();
    } else {
        saved_height = sh->getFloatArray();
    }
    return saved_height;
}

static void getSurroundings(const BlockPos& pos, BlockPtr *neigh)
{
    BlockPos npos[3][3][3];
    int i=0;
    for (int y=-1; y<=1; y++) {
        for (int z=-1; z<=1; z++) {
            for (int x=-1; x<=1; x++) {
                npos[y+1][z+1][x+1] = pos.offset(x, y, z);
            }
        }
    }
    World::instance.getBlocks((BlockPos *)npos, 27, neigh, World::NoLoad);
}

// bool isDirt(BlockPtr p)
// {
//     return p && p->impl == this;
// }

bool DynamicDirtBlock::isSolidBlock(BlockPtr p) 
{
    return p && p->impl != this;
}

bool DynamicDirtBlock::isSoftDirt(BlockPtr p)
{
    return p && p->impl == this && p->getMesh() != p->getDefaultMesh();
}


void DynamicDirtBlock::updateEvent(Block *block)
{
    repaintEvent(block);
}

constexpr double vert = 5;
constexpr double frac = 1.0 / 16.0;

void DynamicDirtBlock::repaintEvent(Block *block)
{
    BlockPtr neigh[3][3][3];
    getSurroundings(block->pos, (BlockPtr *)neigh);
    
    // if (neigh[2][1][1] || (isSolidBlock(neigh[1][0][1]) && isSolidBlock(neigh[1][2][1])) || (isSolidBlock(neigh[1][1][0]) && isSolidBlock(neigh[1][1][2]))) {
    if (neigh[2][1][1] || (neigh[1][0][0] && neigh[1][0][1] && neigh[1][0][2] && neigh[1][1][2] && neigh[1][2][2] && neigh[1][2][1] && neigh[1][2][0] && neigh[1][1][0])) {
        if (block->getMesh() != block->getDefaultMesh()) {
            // Surrounded on all sides: use default
            block->setMesh(0);
            block->setData(0);
            World::instance.repaintSurroundingBlocks(block->pos);
        }
        return;
    }
    
    Spline2D s;
    
    s.z11 = 0.8;
    if (neigh[1][0][1] && neigh[1][1][0] && neigh[1][0][0]) {
        if (isSoftDirt(neigh[1][0][1]) && isSoftDirt(neigh[1][1][0]) && isSoftDirt(neigh[1][0][0])) {
            s.z00 = 0.8;
        } else {
            s.z00 = 1;
        }
    } else if (neigh[1][0][1] && neigh[1][1][0]) {
        if (isSoftDirt(neigh[1][0][1]) && isSoftDirt(neigh[1][1][0])) {
            s.z00 = 0;
        } else {
            s.z00 = 1;
        }
    } else if (neigh[1][0][0] && !neigh[1][0][1] && !neigh[1][1][0]) {
        if (isSoftDirt(neigh[1][0][0])) {
            s.z00 = 0.8;
        } else {
            s.z00 = 1;
        }
        s.z11 = 0.4;
        s.z10 = 0.5;
        s.z01 = 0.5;
    }
    if (neigh[1][0][2] && neigh[1][0][1] && neigh[1][1][2]) {
        if (isSoftDirt(neigh[1][0][1]) && isSoftDirt(neigh[1][1][2]) && isSoftDirt(neigh[1][0][2])) {
            s.z20 = 0.8;
        } else {
            s.z20 = 1;
        }
    } else if (neigh[1][0][1] && neigh[1][1][2]) {
        if (isSoftDirt(neigh[1][0][1]) && isSoftDirt(neigh[1][1][2])) {
            s.z20 = 0;
        } else {
            s.z20 = 1;
        }
    } else if (neigh[1][0][2] && !neigh[1][0][1] && !neigh[1][1][2]) {
        if (isSoftDirt(neigh[1][0][2])) {
            s.z20 = 0.8;
        } else {
            s.z20 = 1;
        }
        s.z11 = 0.4;
        s.z10 = 0.5;
        s.z21 = 0.5;
    }
    if (neigh[1][1][0] && neigh[1][2][1] && neigh[1][2][0]) {
        if (isSoftDirt(neigh[1][2][0]) && isSoftDirt(neigh[1][1][0]) && isSoftDirt(neigh[1][2][1])) {
            s.z02 = 0.8;
        } else {
            s.z02 = 1;
        }
    } else if (neigh[1][1][0] && neigh[1][2][1]) {
        if (isSoftDirt(neigh[1][1][0]) && isSoftDirt(neigh[1][2][1])) {
            s.z02 = 0;
        } else {
            s.z02 = 1;
        }
    } else if (neigh[1][2][0] && !neigh[1][1][0] && !neigh[1][2][1]) {
        if (isSoftDirt(neigh[1][2][0])) {
            s.z02 = 0.8;
        } else {
            s.z02 = 1;
        }
        s.z11 = 0.4;
        s.z12 = 0.5;
        s.z01 = 0.5;
    }
    if (neigh[1][1][2] && neigh[1][2][1] && neigh[1][2][2]) {
        if (isSoftDirt(neigh[1][1][2]) && isSoftDirt(neigh[1][2][1]) && isSoftDirt(neigh[1][2][2])) {
            s.z22 = 0.8;
        } else {
            s.z22 = 1;
        }
    } else if (neigh[1][1][2] && neigh[1][2][1]) {
        if (isSoftDirt(neigh[1][1][2]) && isSoftDirt(neigh[1][2][1])) {
            s.z22 = 0;
        } else {
            s.z22 = 1;
        }
    } else if (neigh[1][2][2] && !neigh[1][1][2] && !neigh[1][2][1]) {
        if (isSoftDirt(neigh[1][2][2])) {
            s.z22 = 0.8;
        } else {
            s.z22 = 1;
        }
        s.z11 = 0.4;
        s.z12 = 0.5;
        s.z21 = 0.5;
    }
    
    if (neigh[1][0][1]) {
        if (isSoftDirt(neigh[1][0][1])) {
            s.z10 = 0.8;
        } else {
            s.z00 = 1;
            s.z10 = 1;
            s.z20 = 1;
            s.z01 = std::max(s.z01, 0.5);
            s.z21 = std::max(s.z21, 0.5);
            // s.z11 = (s.z21 + s.z01) * 0.5;
            s.z11 = std::max((s.z21 + s.z01) * 0.6, (s.z10 + s.z12) * 0.6);
        }
    }
    if (neigh[1][1][0]) {
        if (isSoftDirt(neigh[1][1][0])) {
            s.z01 = 0.8;
        } else {
            s.z00 = 1;
            s.z01 = 1;
            s.z02 = 1;
            s.z10 = std::max(s.z10, 0.5);
            s.z12 = std::max(s.z12, 0.5);
            // s.z11 = (s.z10 + s.z12) * 0.5;
            s.z11 = std::max((s.z21 + s.z01) * 0.6, (s.z10 + s.z12) * 0.6);
        }
    } 
    if (neigh[1][1][2]) {
        if (isSoftDirt(neigh[1][1][2])) {
            s.z21 = 0.8;
        } else {
            s.z20 = 1;
            s.z21 = 1;
            s.z22 = 1;
            s.z10 = std::max(s.z10, 0.5);
            s.z12 = std::max(s.z12, 0.5);
            // s.z11 = (s.z10 + s.z12) * 0.5;
            s.z11 = std::max((s.z21 + s.z01) * 0.6, (s.z10 + s.z12) * 0.6);
        }
    }
    if (neigh[1][2][1]) {
        if (isSoftDirt(neigh[1][2][1])) {
            s.z12 = 0.8;
        } else {
            s.z02 = 1;
            s.z12 = 1;
            s.z22 = 1;
            s.z01 = std::max(s.z01, 0.5);
            s.z21 = std::max(s.z21, 0.5);
//            s.z11 = (s.z21 + s.z01) * 0.5;
            s.z11 = std::max((s.z21 + s.z01) * 0.6, (s.z10 + s.z12) * 0.6);
        }
    }
    
    int neigh_flags = 0;
    if (neigh[1][0][0]) neigh_flags |= 1;
    if (neigh[1][0][1]) neigh_flags |= 2;
    if (neigh[1][0][2]) neigh_flags |= 4;
    if (neigh[1][1][0]) neigh_flags |= 8;
    if (neigh[1][1][2]) neigh_flags |= 16;
    if (neigh[1][2][0]) neigh_flags |= 32;
    if (neigh[1][2][1]) neigh_flags |= 64;
    if (neigh[1][2][2]) neigh_flags |= 128;
    
    if (neigh_flags == 255) s.z11 = 0.8;
    // if (neigh_flags == 165) s.z11 = 0.8;
    
    if (s.z00 == 1) { // NW up
        if (neigh[2][0][1]) { // N
            if (isSoftDirt(neigh[2][0][1])) {
                s.vx0dy0 = -1;
            } else {
                s.vx0dy0 = -1;
            }
        }
        if (neigh[2][1][0]) { // W
            if (isSoftDirt(neigh[2][1][0])) {
                s.vy0dx0 = -1;
            } else {
                s.vy0dx0 = -1;
            }
        }
    } else if (s.z00 == 0) { // NW down
        if (neigh[0][0][1]) { // N
            if (isSoftDirt(neigh[0][0][1])) {
                s.vx0dy0 = 1;
            }
        } else {
            s.vx0dy0 = 1;
        }
        if (neigh[0][1][0]) { // W
            if (isSoftDirt(neigh[0][1][0])) {
                s.vy0dx0 = 1;
            }
        } else {
            s.vy0dx0 = 1;
        }
    }
    
    if (s.z20 == 1) { // NE up
        if (neigh[2][0][1]) { // N
            if (isSoftDirt(neigh[2][0][1])) {
                s.vx2dy0 = -1;
            } else {
                s.vx2dy0 = -1;
            }
        }
        if (neigh[2][1][2]) { // E
            if (isSoftDirt(neigh[2][1][2])) {
                s.vy0dx2 = 1;
            } else {
                s.vy0dx2 = 1;
            }
        }
    } else if (s.z20 == 0) { // NE down
        if (neigh[0][0][1]) { // N
            if (isSoftDirt(neigh[0][0][1])) {
                s.vx2dy0 = 1;
            }
        } else {
            s.vx2dy0 = 1;
        }
        if (neigh[0][1][2]) { // E
            if (isSoftDirt(neigh[0][1][2])) {
                s.vy0dx2 = -1;
            }
        } else {
            s.vy0dx2 = -1;
        }
    }
    
    if (s.z02 == 1) { // SW up
        if (neigh[2][2][1]) { // S
            if (isSoftDirt(neigh[2][2][1])) {
                s.vx0dy2 = 1;
            } else {
                s.vx0dy2 = 1;
            }
        }
        if (neigh[2][1][0]) { // W
            if (isSoftDirt(neigh[2][1][0])) {
                s.vy2dx0 = -1;
            } else {
                s.vy2dx0 = -1;
            }
        }
    } else if (s.z02 == 0) { // SW down
        if (neigh[0][2][1]) { // S
            if (isSoftDirt(neigh[0][2][1])) {
                s.vx0dy2 = -1;
            }
        } else {
            s.vx0dy2 = -1;
        }
        if (neigh[0][1][0]) { // W
            if (isSoftDirt(neigh[0][1][0])) {
                s.vy2dx0 = 1;
            }
        } else {
            s.vy2dx0 = 1;
        }
    } 
    
    if (s.z22 == 1) { // SE up
        if (neigh[2][2][1]) { // S
            if (isSoftDirt(neigh[2][2][1])) {
                s.vx2dy2 = 1;
            } else {
                s.vx2dy2 = 1;
            }
        }
        if (neigh[2][1][2]) { // E
            if (isSoftDirt(neigh[2][1][2])) {
                s.vy2dx2 = 1;
            } else {
                s.vy2dx2 = 1;
            }
        }
    } else if (s.z22 == 0) { // SE down
        if (neigh[0][2][1]) { // S
            if (isSoftDirt(neigh[0][2][1])) {
                s.vx2dy2 = -1;
            }
        } else {
            s.vx2dy2 = -1;
        }
        if (neigh[0][1][2]) { // E
            if (isSoftDirt(neigh[0][1][2])) {
                s.vy2dx2 = -1;
            }
        } else {
            s.vy2dx2 = -1;
        }
    }    
    
    if (s.z10 == 1) { // N up
        if (neigh[2][0][1]) { // N
            if (isSoftDirt(neigh[2][0][1])) {
                s.vx1dy0 = -1;
            } else {
                s.vx1dy0 = -1;
            }
        }
    } else if (s.z10 == 0) { // N down
        if (neigh[0][0][1]) { // N
            if (isSoftDirt(neigh[0][0][1])) {
                s.vx1dy0 = 1;
            }
        } else {
            s.vx1dy0 = 1;
        }
    }
    
    if (s.z21 == 1) { // E up
        if (neigh[2][1][2]) { // E
            if (isSoftDirt(neigh[2][1][2])) {
                s.vy1dx2 = 1;
            } else {
                s.vy1dx2 = 1;
            }
        }
    } else if (s.z21 == 0) { // E down
        if (neigh[0][1][2]) { // E
            if (isSoftDirt(neigh[0][1][2])) {
                s.vy1dx2 = -1;
            }
        } else {
            s.vy1dx2 = -1;
        }
    }
    
    if (s.z01 == 1) { // W up
        if (neigh[2][1][0]) { // W
            if (isSoftDirt(neigh[2][1][0])) {
                s.vy1dx0 = -1;
            } else {
                s.vy1dx0 = -1;
            }
        }
    } else if (s.z01 == 0) { // W down
        if (neigh[0][1][0]) { // W
            if (isSoftDirt(neigh[0][1][0])) {
                s.vy1dx0 = 1;
            }
        } else {
            s.vy1dx0 = 1;
        }
    }
    
    if (s.z12 == 1) { // S up
        if (neigh[2][2][1]) { // S
            if (isSoftDirt(neigh[2][2][1])) {
                s.vx1dy2 = 1;
            } else {
                s.vx1dy2 = 1;
            }
        }
    } else if (s.z12 == 0) { // S down
        if (neigh[0][2][1]) { // S
            if (isSoftDirt(neigh[0][2][1])) {
                s.vx1dy2 = -1;
            }
        } else {
            s.vx1dy2 = -1;
        }
    }
    
    s.compute_coefficients();
    unsigned int seed = block->pos.X ^ block->pos.Y ^ block->pos.Z;
    std::minstd_rand generator (seed);
    std::normal_distribution<double> rnd(0,1);
    
    // std::cout << rnd(generator) << std::endl;
    // std::minstd_rand generator2 (seed);
    // // generator2.seed(seed);
    // std::normal_distribution<double> rnd2(0,1);
    // std::cout << rnd2(generator2) << std::endl;
    
    
    float height[17][17];
    float height2[17][17];
    for (int zi=0; zi<=16; zi++) {
        for (int xi=0; xi<=16; xi++) {
            height[zi][xi] = s.compute(xi * frac, zi * frac);
            height2[zi][xi] = height[zi][xi] + rnd(generator) / 64;
            if (height2[zi][xi]>1) height2[zi][xi]=1;
            if (height[zi][xi]==1) height2[zi][xi]=1;
            if (height2[zi][xi]<0) height2[zi][xi]=0;
            if (height[zi][xi]==0) height2[zi][xi]=0;
        }
    }
    
    float *saved_height = getHeights(block);
    if (block->getMesh() != block->getDefaultMesh() && heights_equal((float *)height, saved_height)) {
        return;
    }
    
    // std::cout << "Recomputing\n";
    // Build new mesh
    MeshPtr newMesh = Mesh::makeMesh();
    newMesh->setTexture(default_mesh->getTextureIndex());
    newMesh->setSolidFaces(facing::DOWN_MASK);
    *(newMesh->getFace(facing::DOWN)) = *(default_mesh->getFace(facing::DOWN));
    
    Face *f;
    for (int zi=0; zi<16; zi++) {
        for (int xi=0; xi<16; xi++) {
            double a = height2[zi][xi];
            double b = height2[zi][xi+1];
            double c = height2[zi+1][xi];
            double d = height2[zi+1][xi+1];
            
            // double max_h = std::max(std::max(a, b), std::max(c, d));
            double max_h = (a + b + c + d) * 0.25;
            max_h = round(max_h*32) * 0.03125;
            geom::Box box(xi*frac, max_h, zi*frac, (xi+1)*frac, 0, (zi+1)*frac);
            // std::cout << xi << "," << zi << " " << box.toString() << std::endl;
            newMesh->addCollisionBox(box);
            
            // a = b = c = d = max_h;
            
            f = newMesh->addFace();
            f->addVertex(xi*frac, a, zi*frac);
            f->addVertex(xi*frac, c, (zi+1)*frac);
            f->addVertex((xi+1)*frac, b, zi*frac);
            f->addTexCoord(xi*frac, 1 - zi*frac);
            f->addTexCoord(xi*frac, 1 - (zi+1)*frac);
            f->addTexCoord((xi+1)*frac, 1 - zi*frac);
    
            f = newMesh->addFace();
            f->addVertex(xi*frac, c, (zi+1)*frac);
            f->addVertex((xi+1)*frac, d, (zi+1)*frac);
            f->addVertex((xi+1)*frac, b, zi*frac);
            f->addTexCoord(xi*frac, 1 - (zi+1)*frac);
            f->addTexCoord((xi+1)*frac, 1 - (zi+1)*frac);
            f->addTexCoord((xi+1)*frac, 1 - zi*frac);
        }
    }
    
    // North faces
    for (int xi=16; xi>0; xi--) {
        const int zi = 0;
        double a = height2[zi][xi];
        double b = height2[zi][xi-1];
        if (a>0 || b>0) {
            f = newMesh->addFace();
            if (a>0) f->addVertex(xi*frac, a, zi*frac);
                     f->addVertex(xi*frac, 0, zi*frac);
                     f->addVertex((xi-1)*frac, 0, zi*frac);
            if (b>0) f->addVertex((xi-1)*frac, b, zi*frac);
            if (a>0) f->addTexCoord((16-xi)*frac, a);
                     f->addTexCoord((16-xi)*frac, 0);
                     f->addTexCoord((17-xi)*frac, 0);
            if (b>0) f->addTexCoord((17-xi)*frac, b);
        }
    }

    // West faces
    for (int zi=0; zi<16; zi++) {
        const int xi = 0;
        double a = height2[zi][xi];
        double b = height2[zi+1][xi];
        if (a>0 || b>0) {
            f = newMesh->addFace();
            if (a>0) f->addVertex(xi*frac, a, zi*frac);
                     f->addVertex(xi*frac, 0, zi*frac);
                     f->addVertex(xi*frac, 0, (zi+1)*frac);
            if (b>0) f->addVertex(xi*frac, b, (zi+1)*frac);
            if (a>0) f->addTexCoord(zi*frac, a);
                     f->addTexCoord(zi*frac, 0);
                     f->addTexCoord((zi+1)*frac, 0);
            if (b>0) f->addTexCoord((zi+1)*frac, b);
        }
    }
    
    // South faces
    for (int xi=0; xi<16; xi++) {
        const int zi = 16;
        double a = height2[zi][xi];
        double b = height2[zi][xi+1];
        if (a>0 || b>0) {
            f = newMesh->addFace();
            if (a>0) f->addVertex(xi*frac, a, zi*frac);
                     f->addVertex(xi*frac, 0, zi*frac);
                     f->addVertex((xi+1)*frac, 0, zi*frac);
            if (b>0) f->addVertex((xi+1)*frac, b, zi*frac);
            if (a>0) f->addTexCoord(xi*frac, a);
                     f->addTexCoord(xi*frac, 0);
                     f->addTexCoord((xi+1)*frac, 0);
            if (b>0) f->addTexCoord((xi+1)*frac, b);
        }
    }
    
    // East faces
    for (int zi=16; zi>0; zi--) {
        const int xi = 16;
        double a = height2[zi][xi];
        double b = height2[zi-1][xi];
        if (a>0 || b>0) {
            f = newMesh->addFace();
            if (a>0) f->addVertex(xi*frac, a, zi*frac);
                     f->addVertex(xi*frac, 0, zi*frac);
                     f->addVertex(xi*frac, 0, (zi-1)*frac);
            if (b>0) f->addVertex(xi*frac, b, (zi-1)*frac);
            if (a>0) f->addTexCoord((16-zi)*frac, a);
                     f->addTexCoord((16-zi)*frac, 0);
                     f->addTexCoord((17-zi)*frac, 0);
            if (b>0) f->addTexCoord((17-zi)*frac, b);
        }
    }    
    
    memcpy(saved_height, height, sizeof(float) * 17*17);
    // block->markDataModified(); // Not very important that the chunk be saved over this
    block->setMesh(newMesh);
    World::instance.repaintSurroundingBlocks(block->pos);    
}


#if 0
void DynamicDirtBlock::updateEvent(Block *block)
{
    // block->setMesh(default_mesh);
    // std::cout << "Update\n";
        
    BlockPtr neigh[8];
    World::instance.getSurroundingBlocks(block->pos, neigh, false, World::NoLoad);
    BlockPtr up = World::instance.getBlock(block->pos.up());
    
    
    if (up || (neigh[0] && neigh[2] && neigh[5] && neigh[7])) {
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
    f->addTexCoord(0.5, height[3]); // weird lighting

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
#endif

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
