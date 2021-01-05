#include "geometry.hpp"
#include "facing.hpp"
#include <iostream>
#include <glm/gtx/string_cast.hpp>
#include "compat.hpp"
#include <limits>

static const glm::dvec3 inward_normal[] = {
    glm::dvec3(0.0, 1.0, 0.0),  // DOWN
    glm::dvec3(0.0, -1.0, 0.0),   // UP
    glm::dvec3(0.0, 0.0, 1.0),  // NORTH
    glm::dvec3(0.0, 0.0, -1.0),   // SOUTH
    glm::dvec3(1.0, 0.0, 0.0),  // WEST
    glm::dvec3(-1.0, 0.0, 0.0)    // EAST
};

static const glm::dvec3 inward_corner[] = {
    glm::dvec3(0.0, 0.0, 0.0),  // DOWN
    glm::dvec3(0.0, 1.0, 1.0),   // UP
    glm::dvec3(0.0, 1.0, 0.0),  // NORTH
    glm::dvec3(1.0, 1.0, 1.0),   // SOUTH
    glm::dvec3(0.0, 1.0, 1.0),  // WEST
    glm::dvec3(1.0, 1.0, 0.0)    // EAST
};

static const glm::dvec3 outward_normal[] = {
    glm::dvec3(0.0, -1.0, 0.0),  // DOWN
    glm::dvec3(0.0, 1.0, 0.0),   // UP
    glm::dvec3(0.0, 0.0, -1.0),  // NORTH
    glm::dvec3(0.0, 0.0, 1.0),   // SOUTH
    glm::dvec3(-1.0, 0.0, 0.0),  // WEST
    glm::dvec3(1.0, 0.0, 0.0)    // EAST
};

static const glm::dvec3 outward_corner[] = {
    glm::dvec3(0.0, 0.0, 1.0),  // DOWN
    glm::dvec3(0.0, 1.0, 0.0),   // UP
    glm::dvec3(1.0, 1.0, 0.0),  // NORTH
    glm::dvec3(0.0, 1.0, 1.0),   // SOUTH
    glm::dvec3(0.0, 1.0, 0.0),  // WEST
    glm::dvec3(1.0, 1.0, 1.0)    // EAST
};

static double distanceToInnerFace(const glm::dvec3& camera, const glm::dvec3& forward, int face)
{
    const glm::dvec3& normal(inward_normal[face]);
    double denom = glm::dot(normal, forward);
    if (denom >= 0) return -1;
    double numer = glm::dot(normal, inward_corner[face] - camera);
    return numer / denom;
}

static double distanceToOuterFace(const glm::dvec3& camera, const glm::dvec3& forward, const glm::dvec3& block_pos, int face)
{
    const glm::dvec3& normal(outward_normal[face]);
    double denom = glm::dot(normal, forward);
    if (denom >= 0) return -1;
    double numer = glm::dot(normal, outward_corner[face] + block_pos - camera);
    return numer / denom;
}

int geom::exitFace(const glm::dvec3& camera_in, const glm::dvec3& forward, double& r)
{
    BlockPos block = geom::whichBlock(camera_in, forward);
    glm::dvec3 camera;
    camera.x = camera_in.x - block.X;
    camera.y = camera_in.y - block.Y;
    camera.z = camera_in.z - block.Z;
    
    // std::cout << "camera_in=" << glm::to_string(camera_in) << " which_block=" << block.toString() << " camera=" << glm::to_string(camera) << " forward=" << glm::to_string(forward) << std::endl;
    double min_d = -1;
    int min_face = -1;
    for (int face=0; face<facing::NUM_FACES; face++) {
        double d = distanceToInnerFace(camera, forward, face);
        // std::cout << "  face " << face << " d=" << d << std::endl;
        if (d <= 0) continue;
        if (min_face<0 || d<min_d) {
            min_face = face;
            min_d = d;
        }
    }
    r = min_d;
    return min_face;
}

int geom::enterFace(const glm::dvec3& camera, const glm::dvec3& forward, const glm::dvec3& block_pos, double &r)
{
    double min_d = -1;
    int min_face = -1;
    for (int face=0; face<facing::NUM_FACES; face++) {
        double d = distanceToOuterFace(camera, forward, block_pos, face);
        if (d < 0) continue;
        if (min_face<0 || d<min_d) {
            min_face = face;
            min_d = d;
        }
    }
    r = min_d;
    return min_face;
}

BlockPos geom::whichBlock(const glm::dvec3& point, const glm::dvec3& forward)
{
    BlockPos pos;
    pos.X = (int)floor(point.x);
    pos.Y = (int)floor(point.y);
    pos.Z = (int)floor(point.z);
    double fracX = point.x - pos.X;
    double fracY = point.y - pos.Y;
    double fracZ = point.z - pos.Z;
    if (fracX<0.001 && forward.x<0) pos.X--;
    if (fracY<0.001 && forward.y<0) pos.Y--;
    if (fracZ<0.001 && forward.z<0) pos.Z--;
    if (fracX>0.999 && forward.x>0) pos.X++;
    if (fracY>0.999 && forward.y>0) pos.Y++;
    if (fracZ>0.999 && forward.z>0) pos.Z++;
    return pos;
}

BlockPos geom::computeCenter(const glm::dvec3& camera_pos)
{
    BlockPos center;
    center.X = double2int(camera_pos.x);
    center.Y = double2int(camera_pos.y);
    center.Z = double2int(camera_pos.z);
    return center;
}

bool geom::Box::intersects(const geom::Box& other) const
{
    if (other.neg.x >= pos.x) return false;
    if (other.neg.y >= pos.y) return false;
    if (other.neg.z >= pos.z) return false;
    if (other.pos.x <= neg.x) return false;
    if (other.pos.y <= neg.y) return false;
    if (other.pos.z <= neg.z) return false;
    return true;
}

geom::Box::Box(const BlockPos& bp)
{
    neg.x = bp.X;
    neg.y = bp.Y;
    neg.z = bp.Z;
    pos.x = bp.X+1;
    pos.y = bp.Y+1;
    pos.z = bp.Z+1;
}

geom::Box::Box(const geom::EntityBox& bb)
{
    double halfw = bb.width*0.5;
    neg.x = bb.position.x - halfw;
    neg.y = bb.position.y;
    neg.z = bb.position.z - halfw;
    pos.x = bb.position.x + halfw;
    pos.y = bb.position.y + bb.height;
    pos.z = bb.position.z + halfw;
}

#if 0
void geom::Box::intBoxes(BlockPosArray& list)
{
    // XXX switch to lrint for performance
    // This whole thing is a stupid slow method
    int extent_x = (int)ceil(pos.x - neg.x) + 1;
    int extent_y = (int)ceil(pos.y - neg.y) + 1;
    int extent_z = (int)ceil(pos.z - neg.z) + 1;
    int base_x = (int)floor(neg.x);
    int base_y = (int)floor(neg.y);
    int base_z = (int)floor(neg.z);
    list.arr.clear();
    for (int x=0; x<extent_x; x++) {
        for (int y=0; y<extent_y; y++) {
            for (int z=0; z<extent_z; z++) {
                BlockPos bp = BlockPos(base_x + x, base_y + y, base_z + z);
                Box intbox = Box(bp);
                if (intersects(intbox)) {
                    list.insert(bp);
                }
            }        
        }
    }
}
#endif

void geom::Box::intBoxes(BlockPosArray& list) const
{
    int base_x = lrint(floor(neg.x));
    int base_y = lrint(floor(neg.y));
    int base_z = lrint(floor(neg.z));
    int end_x  = lrint(ceil(pos.x));
    int end_y  = lrint(ceil(pos.y));
    int end_z  = lrint(ceil(pos.z));
    for (int x=base_x; x<end_x; x++) {
        for (int y=base_y; y<end_y; y++) {
            for (int z=base_z; z<end_z; z++) {
                BlockPos bp(x, y, z);
                list.append(bp);
            }
        }
    }
    list.sort();
}

// Computes multiplier of v (motion) for (b+v) to graze self
// Automatically imposes 0.001 unit margin
// This assumes that the projection would actually intersect, because
// it just does quick plane comparisons
double geom::Box::vectorDistance(const Box& b, const glm::dvec3& v) const
{
    if (intersects(b)) return -1;
    double r = -1;
    
    // a east of b
    if (v.x!=0) {
        double r1 = ((b.pos.x+0.001) - neg.x) / v.x;
        if (r1>=0) {
            if (r<0 || r1<r) r = r1;
        }
    }

    // a west of b
    if (v.x!=0){
        double r1 = ((b.neg.x-0.001) - pos.x) / v.x;
        if (r1>=0) {
            if (r<0 || r1<r) r = r1;
        }
    }
    
    // a above b
    if (v.y!=0) {
        double r1 = ((b.pos.y+0.001) - neg.y) / v.y;
        if (r1>=0) {
            if (r<0 || r1<r) r = r1;
        }
    }

    // a below b
    if (v.y!=0){
        double r1 = ((b.neg.y-0.001) - pos.y) / v.y;
        if (r1>=0) {
            if (r<0 || r1<r) r = r1;
        }
    }
    
    // a south of b
    if (v.z!=0) {
        double r1 = ((b.pos.z+0.001) - neg.z) / v.z;
        if (r1>=0) {
            if (r<0 || r1<r) r = r1;
        }
    }

    // a north of b
    if (v.z!=0){
        double r1 = ((b.neg.z-0.001) - pos.z) / v.z;
        if (r1>=0) {
            if (r<0 || r1<r) r = r1;
        }
    }
    
    return r;
}

geom::BoxArray geom::BoxArray::difference(const BoxArray& other) const
{
    BoxArray out;
    
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

double geom::Box::distance(const Box& b) const
{
    bool x_overlap = !(b.neg.x >= pos.x || b.pos.x <= neg.x);
    bool y_overlap = !(b.neg.y >= pos.y || b.pos.y <= neg.y);
    bool z_overlap = !(b.neg.z >= pos.z || b.pos.z <= neg.z);
    
    if (x_overlap && y_overlap && z_overlap) return -1;
    
    if (x_overlap && y_overlap) {
        if (b.neg.z > pos.z) {
            return b.neg.z - pos.z;
        } else {
            return neg.z - b.pos.z;
        }
    }
    
    if (z_overlap && y_overlap) {
        if (b.neg.x > pos.x) {
            return b.neg.x - pos.x;
        } else {
            return neg.x - b.pos.x;
        }
    }
    
    if (x_overlap && z_overlap) {
        if (b.neg.y > pos.y) {
            return b.neg.y - pos.y;
        } else {
            return neg.y - b.pos.y;
        }
    }
    
    return DBL_MAX;
}


double geom::BoxArray::minDistance(const Box& box) const
{
    double dist = DBL_MAX;
    
    for (auto i=arr.begin(); i!=arr.end(); i++) {
        double d = box.distance(*i);
        if (d < dist) dist = d;
    }
    
    return dist;
}


#if 0
int main()
{
    std::cout << std::fixed;
    
    glm::dvec3 camera(1.1, 0.5, 0.5);
    glm::dvec3 forward(1, 0, 0);
    double r;
    int face = geom::exitFace(camera, forward, r);
    std::cout << "Face:" << facing::faceName(face) << " r=" << r << std::endl;
    
    return 0;
}
#endif
