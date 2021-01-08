#include "position.hpp"


#include <sstream>

std::string BlockPos::toString() const
{
    std::stringstream ss;
    ss << "block(" << X << ',' << Y << ',' << Z << ')';
    return ss.str();
}

std::string ChunkPos::toString() const
{
    std::stringstream ss;
    ss << "chunk(" << X << ',' << Y << ',' << Z << ')';
    return ss.str();
}

void BlockPos::allNeighbors(BlockPos *npos) const
{
    // XXX split out into north, south, etc. for slightly higher performance
    for (int face=0; face<facing::NUM_FACES; face++) {
        npos[face] = neighbor(face);
    }
}

void BlockPos::allSurrounding(BlockPos *npos) const
{
    npos[0] = north().west();
    npos[1] = north();
    npos[2] = north().east();
    npos[3] = west();
    npos[4] = east();
    npos[5] = south().west();
    npos[6] = south();
    npos[7] = south().east();
}


BlockPosArray BlockPosArray::difference(const BlockPosArray& other) const
{
    BlockPosArray out;
    
    int i = 0;
    int j = 0;
    while (i<arr.size() && j<other.arr.size()) {
        if (arr[i] < other.arr[j]) {
            out.arr.push_back(arr[i]);
            i++;
            continue;
        }
        if (other.arr[j] < arr[i]) {
            j++;
            continue;
        }
        i++;
        j++;
    }
    while (i<arr.size()) {
        out.arr.push_back(arr[i]);
        i++;
    }
    
    return out;
}
