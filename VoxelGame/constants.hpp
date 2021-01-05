#ifndef _INCLUDED_CONSTANTS_HPP
#define _INCLUDED_CONSTANTS_HPP

#include <ctype.h>

namespace packing {
    constexpr int blockX_size = 26;
    constexpr int blockZ_size = 26;
    constexpr int blockY_size = 12;
    
    constexpr int blockX_shift = 0;
    constexpr int blockZ_shift = blockX_size;
    constexpr int blockY_shift = blockX_size + blockZ_size;
    
    constexpr uint64_t blockX_mask = (1<<blockX_size)-1;
    constexpr uint64_t blockZ_mask = (1<<blockZ_size)-1;
    constexpr uint64_t blockY_mask = (1<<blockY_size)-1;
    
    
    constexpr int chunkX_size = blockX_size - 4;
    constexpr int chunkZ_size = blockZ_size - 4;
    constexpr int chunkY_size = blockY_size - 4;
    
    constexpr int chunkX_shift = 0;
    constexpr int chunkZ_shift = chunkX_size;
    constexpr int chunkY_shift = chunkX_size + chunkZ_size;
    
    constexpr uint64_t chunkX_mask = (1<<chunkX_size)-1;
    constexpr uint64_t chunkZ_mask = (1<<chunkZ_size)-1;
    constexpr uint64_t chunkY_mask = (1<<chunkY_size)-1;
}

namespace sizes {
    constexpr int chunk_storage_size = 4096;
}

#endif