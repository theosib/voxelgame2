

#include "blocktype.hpp"
#include "blocklibrary.hpp"

class StaticCubeBlock : public BlockType {
private:
    MeshPtr default_mesh;
    std::string name;
    
public:
    StaticCubeBlock(const std::string& name);
    virtual ~StaticCubeBlock();
    
    // Get default mesh
    virtual MeshPtr getMesh();
    
    virtual bool hitAction(Block *block, int face);
    virtual bool useAction(Block *block, int face);
    virtual void tickEvent(Block *block, int tick_types);
    virtual void placeEvent(Block *block);
    virtual void breakEvent(Block *block);
    virtual void updateEvent(Block *block);

    virtual const std::string& getName();
};

StaticCubeBlock::StaticCubeBlock(const std::string& name)
{
    default_mesh = Mesh::makeMesh();
    default_mesh->loadMesh(name);
    this->name = name;
    // BlockLibrary::instance.registerBlockType(name, this);
}

StaticCubeBlock::~StaticCubeBlock() {}

MeshPtr StaticCubeBlock::getMesh()
{
    return default_mesh;
}

bool StaticCubeBlock::hitAction(Block *block, int face) { return BlockType::DefaultAction; }
bool StaticCubeBlock::useAction(Block *block, int face) { return BlockType::DefaultAction; }
void StaticCubeBlock::tickEvent(Block *block, int tick_types) {}
void StaticCubeBlock::placeEvent(Block *block) {}
void StaticCubeBlock::breakEvent(Block *block) {}
void StaticCubeBlock::updateEvent(Block *block) {}

const std::string& StaticCubeBlock::getName()
{
    return name;
}

static std::vector<StaticCubeBlock*> static_blocks;

static void init_static_block(const std::string& name)
{
    StaticCubeBlock *ptr = new StaticCubeBlock(name);
    BlockLibrary::instance.registerBlockType(name, ptr);
}

void register_static_blocks()
{
    init_static_block("wood");
    init_static_block("wood_wedge");
    init_static_block("wood_outer_wedge");
    init_static_block("wood_inner_wedge");
    init_static_block("wood_slab");
    init_static_block("wood_diag");
    init_static_block("steel");
    init_static_block("dirt");
    init_static_block("cobblestone");
    init_static_block("stone");
    init_static_block("concrete");
    init_static_block("stars");
    init_static_block("brick");
    init_static_block("marble");
    init_static_block("chicken");
    init_static_block("carpet");
    init_static_block("transgray");
}

