#include "blocklibrary.hpp"

BlockLibrary BlockLibrary::instance;

BlockLibrary::BlockLibrary()
{
    block_index["air"] = 0;
}

void BlockLibrary::getBlockTypes(std::vector<BlockType *>& list)
{
    list.clear();
    for (auto i=block_index.begin(); i!=block_index.end(); ++i) {
        list.push_back(i->second);
    }
}

void BlockLibrary::getBlockNames(std::vector<std::string>& list)
{
    list.clear();
    for (auto i=block_index.begin(); i!=block_index.end(); ++i) {
        if (i->second)
            list.push_back(i->first);
    }
}
