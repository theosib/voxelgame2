#ifndef INCLUDED_GEOMETRY_HPP
#define INCLUDED_GEOMETRY_HPP

#include <glm/glm.hpp>
#include <math.h>
#include "position.hpp"
#include <algorithm>

namespace geom {
    
    inline double intersectDistance(const glm::dvec3& camera, const glm::dvec3& forward, const glm::dvec3& plane_normal, const glm::dvec3& plane_corner) {
        return glm::dot(plane_normal, plane_corner - camera) / glm::dot(plane_normal, forward);
    }
        
    inline const glm::dvec3 projectForward(const glm::dvec3& camera, const glm::dvec3& forward, double r) {
        return camera + r * forward;
    }
    
    inline const glm::dvec3 blockCorner(const glm::dvec3& point) {
        return glm::dvec3(floor(point.x), floor(point.y), floor(point.z));
    }
    
    BlockPos whichBlock(const glm::dvec3& point, const glm::dvec3& forward);
    
    int exitFace(const glm::dvec3& camera_in, const glm::dvec3& forward, double& r);
    int enterFace(const glm::dvec3& camera, const glm::dvec3& forward, const glm::dvec3& block_pos, double &r);
    
    BlockPos computeCenter(const glm::dvec3& camera_pos);
    
    
    struct EntityBox {
        glm::dvec3 position; // bottom center
        double width, height;
    };
    
    struct BoxArray;
    
    struct Box {
        glm::dvec3 neg; // Negatives for all coordinates
        glm::dvec3 pos; // Positives for all coordinates
        
        Box() {}
        Box(const Box& bx) {
            neg = bx.neg;
            pos = bx.pos;
        }
        Box(const BlockPos& bp);
        Box(const EntityBox& bb);
        Box(double x1, double y1, double z1, double x2, double y2, double z2) {
            if (x1 > x2) std::swap(x1, x2);
            if (y1 > y2) std::swap(y1, y2);
            if (z1 > z2) std::swap(z1, z2);
            neg.x = x1;
            neg.y = y1;
            neg.z = z1;
            pos.x = x2;
            pos.y = y2;
            pos.z = z2;
        }
        Box(float *p) : Box(p[0], p[1], p[2], p[3], p[4], p[5]) {}
        Box(double *p) : Box(p[0], p[1], p[2], p[3], p[4], p[5]) {}
        
        Box offset(glm::dvec3 motion) const {
            Box b;
            b.neg = neg + motion;
            b.pos = pos + motion;
            return b;
        }
        
        Box offset(double offsetX, double offsetY, double offsetZ) const {
            Box b;
            b.neg.x = neg.x + offsetX;
            b.neg.y = neg.y + offsetY;
            b.neg.z = neg.z + offsetZ;
            b.pos.x = pos.x + offsetX;
            b.pos.y = pos.y + offsetY;
            b.pos.z = pos.z + offsetZ;
            return b;
        }
        
        Box rotate(const glm::dmat4& rot) const {
            Box b;
            b.neg = rot * glm::dvec4(neg, 1.0);
            b.pos = rot * glm::dvec4(pos, 1.0);
            if (b.neg.x > b.pos.x) std::swap(b.neg.x, b.pos.x);
            if (b.neg.y > b.pos.y) std::swap(b.neg.y, b.pos.y);
            if (b.neg.z > b.pos.z) std::swap(b.neg.z, b.pos.z);
            return b;
        }
    
        // Do this and other box intersect?
        bool intersects(const Box& other) const;
        
        double vectorDistance(const Box& other, const glm::dvec3& motion) const;
        
        // Produce list of integer-aligned boxes
        void intBoxes(BlockPosArray& list) const;
        
        bool operator<(const Box& other) const {
            if (neg.y < other.neg.y) return true;
            if (neg.y > other.neg.y) return false;
            if (neg.x < other.neg.x) return true;
            if (neg.x > other.neg.x) return false;
            if (neg.z < other.neg.z) return true;
            if (neg.z > other.neg.z) return false;
            return false;
        }
        
        bool operator==(const Box& other) const {
            return neg.x == other.neg.x && neg.y == other.neg.y && neg.z == other.neg.z &&
                   pos.x == other.pos.x && pos.y == other.pos.y && pos.z == other.pos.z;
        }
        
        double distance(const Box& other) const;
        
        std::string toString() const;
    };
    
    struct BoxArray {
        std::vector<Box> arr;
        void insert(const Box& b) {
            arr.insert(std::upper_bound(arr.begin(), arr.end(), b), b);
        }
        void append(const Box& b) {
            arr.push_back(b);
        }
        void sort() {
            std::sort(arr.begin(), arr.end());
        }
        void clear() {
            arr.clear();
        }
        
        BoxArray difference(const BoxArray& other) const;
        
        double minDistance(const Box& box) const;
    };
}

#endif
