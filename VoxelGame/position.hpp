#ifndef _INCLUDED_TYPES_HPP
#define _INCLUDED_TYPES_HPP

#include <string>
#include "constants.hpp"
#include "facing.hpp"
#include <vector>
#include <algorithm>


struct ChunkPos {
    int32_t X, Y, Z;
    
    ChunkPos() {}
    ChunkPos(int32_t x, int32_t y, int32_t z) : X(x), Y(y), Z(z) {}
    
    uint64_t packed() const {
        uint64_t x = X & packing::chunkX_mask;
        uint64_t y = Y & packing::chunkY_mask;
        uint64_t z = Z & packing::chunkZ_mask;
        return (x << packing::chunkX_shift) | (y << packing::chunkY_shift) | (z << packing::chunkZ_shift);
    }
    
    std::string toString() const;
    
    bool operator ==(const ChunkPos& other) const {
        return X == other.X && Y == other.Y && Z == other.Z;
    }
    
    bool operator<(const ChunkPos& other) const {
        if (Y < other.Y) return true;
        if (Y > other.Y) return false;
        if (X < other.X) return true;
        if (X > other.X) return false;
        if (Z < other.Z) return true;
        if (Z > other.Z) return false;
        return false;
    }        
    
    ChunkPos down(int dist=1) const { return ChunkPos(X, Y-dist, Z); }
    ChunkPos up(int dist=1) const { return ChunkPos(X, Y+dist, Z); }
    ChunkPos north(int dist=1) const { return ChunkPos(X, Y, Z-dist); }
    ChunkPos south(int dist=1) const { return ChunkPos(X, Y, Z+dist); }
    ChunkPos west(int dist=1) const { return ChunkPos(X-dist, Y, Z); }
    ChunkPos east(int dist=1) const { return ChunkPos(X+dist, Y, Z); }
    
    ChunkPos neighbor(int face) const {
        const int *vec = facing::int_vector[face];
        return ChunkPos(X+vec[0], Y+vec[1], Z+vec[2]);
    }    
};

struct BlockPos {
    int32_t X, Y, Z;
    
    BlockPos() {}
    BlockPos(int32_t x, int32_t y, int32_t z) : X(x), Y(y), Z(z) {}
    
    ChunkPos getChunkPos() const {
        return ChunkPos(X>>4, Y>>4, Z>>4);
    }
    
    uint64_t packed() const {
        uint64_t x = X & packing::blockX_mask;
        uint64_t y = Y & packing::blockY_mask;
        uint64_t z = Z & packing::blockZ_mask;
        return (x << packing::blockX_shift) | (y << packing::blockY_shift) | (z << packing::blockZ_shift);
    }    
    
    BlockPos down(int dist=1) const { return BlockPos(X, Y-dist, Z); }
    BlockPos up(int dist=1) const { return BlockPos(X, Y+dist, Z); }
    BlockPos north(int dist=1) const { return BlockPos(X, Y, Z-dist); }
    BlockPos south(int dist=1) const { return BlockPos(X, Y, Z+dist); }
    BlockPos west(int dist=1) const { return BlockPos(X-dist, Y, Z); }
    BlockPos east(int dist=1) const { return BlockPos(X+dist, Y, Z); }
    
    BlockPos neighbor(int face) const {
        const int *vec = facing::int_vector[face];
        return BlockPos(X+vec[0], Y+vec[1], Z+vec[2]);
    }
    
    void allNeighbors(BlockPos *npos) const;
    void allSurrounding(BlockPos *npos) const;
    
    std::string toString() const;
    
    bool operator ==(const BlockPos& other) const {
        return X == other.X && Y == other.Y && Z == other.Z;
    }
    
    static BlockPos getBlockPos(const ChunkPos& cp) {
        return BlockPos(cp.X << 4, cp.Y << 4, cp.Z << 4);
    }
    
    bool operator<(const BlockPos& other) const {
        if (Y < other.Y) return true;
        if (Y > other.Y) return false;
        if (X < other.X) return true;
        if (X > other.X) return false;
        if (Z < other.Z) return true;
        if (Z > other.Z) return false;
        return false;
    }    
};

struct BlockPosArray {
    std::vector<BlockPos> arr;
    void insert(const BlockPos& b) {
        arr.insert(std::upper_bound(arr.begin(), arr.end(), b), b);
    }
    void append(const BlockPos& b) {
        arr.push_back(b);
    }
    void sort() {
        std::sort(arr.begin(), arr.end());
    }
    BlockPosArray difference(const BlockPosArray& other) const;
};


namespace std {
    template <>
    struct hash<BlockPos> {
        std::size_t operator()(const BlockPos& k) const {
            return k.packed();
        }
    };
}

#endif