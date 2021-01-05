#ifndef INCLUDED_BLOCK_LIBRARY_HPP
#define INCLUDED_BLOCK_LIBRARY_HPP

#include <unordered_map>
#include "blocktype.hpp"
#include <iostream>
#include <vector>

class BlockLibrary {
public:
    static BlockLibrary instance;
    
private:
    std::unordered_map<std::string, BlockType *> block_index;
        
public:
    BlockLibrary();
    ~BlockLibrary() {};
    
    BlockType* getBlockType(const std::string& name) {
        return block_index[name];
    }
    
    void registerBlockType(const std::string& name, BlockType *bt) {
        std::cout << "Registering " << name << " as " << ((void*)bt) << std::endl;
        block_index[name] = bt;
    }
    
    void getBlockTypes(std::vector<BlockType *>& list);
    void getBlockNames(std::vector<std::string>& list);
};


#endif
