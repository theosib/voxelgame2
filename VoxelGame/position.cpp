#include "position.hpp"
#include <sstream>

BlockPos::BlockPos(const glm::dvec3& v)
{
    X = lrint(floor(v.x));
    Y = lrint(floor(v.y));
    Z = lrint(floor(v.z));
}

BlockPos::BlockPos(const glm::vec3& v)
{
    X = lrint(floor(v.x));
    Y = lrint(floor(v.y));
    Z = lrint(floor(v.z));
}

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
    int i=0;
    for (int y=-1; y<=1; y++) {
        for (int z=-1; z<=1; z++) {
            for (int x=-1; x<=1; x++) {
                if (x==0 && y==0 && z==0) continue;
                npos[i++] = offset(x, y, z);
            }
        }
    }
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
