#include "chunk.hpp"
#include "block.hpp"
#include "blocklibrary.hpp"
#include "chunkview.hpp"
#include <iostream>
#include "filelocator.hpp"
#include <fstream>
#include "time.hpp"
#include "world.hpp"

Chunk::Chunk(ChunkPos p)
{
    chunk_pos = p;
    
    // Add air block
    name2index["air"] = 0;
    index2name.push_back(0);
    
    // clear storage
    memset(block_storage, 0, sizeof(block_storage));
    memset(block_rotation, 0, sizeof(block_rotation));
    
    needs_save = false;
    last_save = ref::currentTime();
    
    //view = std::unique_ptr<ChunkView>(new ChunkView(this));
}

uint16_t Chunk::getBlockID(const std::string& name) {
    auto i = name2index.find(name);
    if (i != name2index.end()) return i->second;
    BlockType *bt = BlockLibrary::instance.getBlockType(name);
    std::cout << "Looking up blocktype for " << name << " got " << ((void*)bt) << std::endl;
    size_t id = index2name.size();
    index2name.push_back(bt);
    name2index[name] = (uint16_t)id;
    return (uint16_t)id;
}

uint16_t Chunk::getBlockID(BlockType *bt)
{
    const std::string& name(bt->getName());
    auto i = name2index.find(name);
    if (i != name2index.end()) return i->second;
    size_t id = index2name.size();
    index2name.push_back(bt);
    name2index[name] = (uint16_t)id;
    return (uint16_t)id;
}

BlockPtr Chunk::getBlock(const BlockPos& pos)
{
    uint16_t index = chunkBlockIndex(pos);
    uint16_t block_id = block_storage[index];
    if (!block_id) return 0; // Air
    BlockType *bt = lookupBlockType(block_id);
    if (!bt) {
        std::cout << "Error: Got null bt for block ID " << block_id << std::endl;
        for (auto i=index2name.begin(); i!=index2name.end(); ++i) {
            std::cout << (*i) << std::endl;
        }
        for (auto i=name2index.begin(); i!=name2index.end(); ++i) {
            std::cout << i->first << " " << i->second << std::endl;
        }
    }
        
    BlockPtr block = Block::makeBlock();
    block->chunk = this;
    block->pos = pos;
    block->storage_index = index;
    block->impl = bt;    
    return block;
}

BlockPtr Chunk::getBlock(uint16_t index)
{
    uint16_t block_id = block_storage[index];
    if (!block_id) return 0; // Air
    BlockType *bt = lookupBlockType(block_id);
        
    BlockPtr block = Block::makeBlock();
    block->chunk = this;
    block->pos = decodeIndex(index);
    block->storage_index = index;
    block->impl = bt;    
    return block;
}


void Chunk::breakBlock(const BlockPos& pos)
{
    setBlock(pos, "air");
}

void Chunk::setBlock(const BlockPos& pos, const std::string& name, int rotation)
{
    BlockPtr old_block = getBlock(pos);
    if (old_block && old_block->impl) {
        old_block->breakEvent();
    }

    uint16_t index = chunkBlockIndex(pos);
    uint16_t block_id = getBlockID(name);
    block_storage[index] = block_id;
    block_rotation[index] = rotation;
    // visible_faces[index] = 0;
    data_containers.erase(index);
    meshes[index] = 0;

    if (block_id) {
        BlockPtr block = getBlock(pos);
        if (block) block->placeEvent();
    }

    if (view) {
        view->block_visual_modified[index] = true;
        view->chunk_visual_modified = true;
    }
    
    std::cout << "Marking block needing save\n";
    needs_save = true;
}

void Chunk::requestVisualUpdate(Block *block)
{
    if (!view) return;
    view->block_visual_modified[block->storage_index] = true;
    view->chunk_visual_modified = true;
}

void Chunk::updateBlock(const BlockPos& pos)
{
    BlockPtr block = getBlock(pos);
    if (block) block->updateEvent();
    
    if (!view) return;
    int index = chunkBlockIndex(pos);
    view->block_visual_modified[index] = true;
    view->chunk_visual_modified = true;
}

void Chunk::updateAllBlocks()
{
    for (int i=0; i<sizes::chunk_storage_size; i++) {
        BlockPos p(decodeIndex(i));
        World::instance.updateBlock(p);
    }
#if 0
    for (int i=0; i<sizes::chunk_storage_size; i++) {
        BlockPtr block = getBlock(i);
        if (block) block->updateEvent();
    }
    
    if (!view) return;
    view->markChunkUpdated();
#endif
}

int Chunk::getVisibleFaces(Block *block)
{ 
    if (!view) return 0;
    return view->block_show_faces[block->storage_index];
}

void Chunk::setVisibleFaces(Block *block, int faces)
{
    if (!view) return;
    view->block_show_faces[block->storage_index] = faces;
    requestVisualUpdate(block);
}

int Chunk::getRotation(Block *block)
{
    return block_rotation[block->storage_index];
}

void Chunk::setRotation(Block *block, int rot)
{
    block_rotation[block->storage_index] = rot;
    requestVisualUpdate(block);
    needs_save = true;
}


MeshPtr Chunk::getMesh(Block *block) {
    int16_t index = block->storage_index;
    return getMesh(index);
}

void Chunk::setMesh(Block *block, MeshPtr mesh) {
    meshes[block->storage_index] = mesh;
    requestVisualUpdate(block);
}


DataContainerPtr Chunk::getDataContainer(Block *block, bool create)
{
    auto i = data_containers.find(block->storage_index);
    if (i != data_containers.end()) return i->second;
    if (!create) return 0;
    DataContainerPtr dc = DataContainer::makeContainer();
    data_containers[block->storage_index] = dc;
    return dc;
}

void Chunk::getCorners(BlockPos *bpos, const BlockPos& center)
{
    BlockPos a = BlockPos::getBlockPos(chunk_pos);
    a.X -= center.X;
    a.Y -= center.Y;
    a.Z -= center.Z;
    bpos[0] = a;
    bpos[1] = BlockPos(a.X + 16, a.Y, a.Z);
    bpos[2] = BlockPos(a.X, a.Y + 16, a.Z);
    bpos[3] = BlockPos(a.X + 16, a.Y + 16, a.Z);
    bpos[4] = BlockPos(a.X, a.Y, a.Z + 16);
    bpos[5] = BlockPos(a.X + 16, a.Y, a.Z + 16);
    bpos[6] = BlockPos(a.X, a.Y + 16, a.Z + 16);
    bpos[7] = BlockPos(a.X + 16, a.Y + 16, a.Z + 16);
}

ChunkView* Chunk::getView()
{
    if (view) return view.get();
    view = std::unique_ptr<ChunkView>(new ChunkView(this));    
    return view.get(); 
}


// XXX either use spinlock on chunk while serializing, or have main thread serialize
void Chunk::save()
{
    if (!needs_save) return;
    needs_save = false;
    
    DataContainerPtr data = DataContainer::makeContainer();
    
    // name/id mapping
    // std::unordered_map<std::string, uint16_t> name2index;
    DataContainerPtr ids = DataContainer::makeContainer();
    data->setNamedItem("ids", DataItem::wrapContainer(ids));
    for (auto i=name2index.begin(); i!=name2index.end(); ++i) {
        ids->setNamedItem(i->first, DataItem::makeInt16(i->second));
    }
    
    // block storage
    // uint16_t block_storage[sizes::chunk_storage_size];
    data->setNamedItem("blocks", DataItem::makeInt16Array(sizes::chunk_storage_size, (const int16_t*)block_storage));
    
    // block rotation
    // uint8_t block_rotation[sizes::chunk_storage_size];
    data->setNamedItem("rotation", DataItem::makeInt8Array(sizes::chunk_storage_size, (const int8_t*)block_rotation));
    
    // Data containers
    // std::unordered_map<uint16_t, DataContainerPtr> data_containers;
    DataContainerPtr ctrs = DataContainer::makeContainer();
    data->setNamedItem("data", DataItem::wrapContainer(ctrs));
    for (auto i=data_containers.begin(); i!=data_containers.end(); ++i) {
        ctrs->setIndexedItem(i->first, DataItem::wrapContainer(i->second));
    }
    
    // data->debug();
    std::deque<char> serial;
    data->pack(serial);
    
    std::string chunk_name = chunk_pos.toString();
    std::string fname = FileLocator::instance.chunk(chunk_name);
    std::cout << "Saving chunk " << fname << std::endl;
    std::ofstream wf(fname, std::ios::binary);
    std::copy(serial.begin(), serial.end(), std::ostreambuf_iterator<char>(wf));
    wf.close();
    
    last_save = ref::currentTime();
    
    // MeshPtr meshes[sizes::chunk_storage_size];
}

bool Chunk::load()
{
    std::string chunk_name = chunk_pos.toString();
    std::string fname = FileLocator::instance.chunk(chunk_name);
    // std::cout << "Loading chunk " << fname << std::endl;
    
    std::ifstream rf(fname, std::ios::binary);
    if (!rf.good()) return false;
    std::deque<char> serial;
    serial.assign(std::istreambuf_iterator<char>(rf), std::istreambuf_iterator<char>());
    rf.close();
    
    // std::cout << serial.size() << " bytes\n";
    // DataContainerPtr data = DataContainer::makeContainer();
    // data->unpack(serial);
    DataContainerPtr data = DataContainer::unpack(serial);
    // data->debug();
    
    // name/id mapping
    // std::unordered_map<std::string, uint16_t> name2index;
    DataContainerPtr ids = data->getNamedItem("ids")->getContainer();
    name2index.clear();
    index2name.clear();
    for (int i=0; i<ids->numItems(); i++) {
        DataItemPtr a = ids->getItem(i);
        const std::string& name(a->getName());
        uint16_t id = a->getInt16();
        name2index[name] = id;
        BlockType *bt = BlockLibrary::instance.getBlockType(name);
        if (id >= index2name.size()) index2name.resize(id+1);
        index2name[id] = bt;
    }
    
    // block storage
    // uint16_t block_storage[sizes::chunk_storage_size];
    DataItemPtr blocks = data->getNamedItem("blocks");
    uint16_t* blocks_ptr = (uint16_t*)(blocks->getInt16Array());
    // std::cout << "Blocks array count=" << blocks->getArrayCount() << std::endl;
    memcpy(block_storage, blocks_ptr, sizes::chunk_storage_size * sizeof(uint16_t));
    
    // block rotation
    // uint8_t block_rotation[sizes::chunk_storage_size];
    DataItemPtr rot = data->getNamedItem("rotation");
    uint8_t* rot_ptr = (uint8_t*)(rot->getInt8Array());
    memcpy(block_rotation, rot_ptr, sizes::chunk_storage_size);
    
    // Data containers
    // std::unordered_map<uint16_t, DataContainerPtr> data_containers;
    DataContainerPtr ctrs = data->getNamedItem("data")->getContainer();
    data_containers.clear();
    for (int i=0; i<ctrs->numItems(); i++) {
        DataItemPtr a = ctrs->getItem(i);
        data_containers[(uint16_t)a->getIndex()] = a->getContainer();
    }
    
    last_save = ref::currentTime();
    
    return true;
}


void Chunk::tickAllBlocks(double elapsed_time)
{
    
}
