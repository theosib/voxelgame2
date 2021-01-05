#ifndef _INCLUDED_FACING_HPP
#define _INCLUDED_FACING_HPP

#include <glm/glm.hpp>
#include <string>

namespace facing {
    enum direction {
        DOWN,   // negative Y
        UP,     // positive Y
        NORTH,  // negative Z
        SOUTH,  // positive Z
        WEST,   // negative X
        EAST,   // positive X
        NUM_FACES
    };
    
    extern const char *face_name[];
    inline const char *faceName(int face) {
        if (face<0 || face>=NUM_FACES) return "unknown";
        return face_name[face];
    }
    
    constexpr int DOWN_MASK = 1<<DOWN;
    constexpr int UP_MASK = 1<<UP;
    constexpr int NORTH_MASK = 1<<NORTH;
    constexpr int SOUTH_MASK = 1<<SOUTH;
    constexpr int WEST_MASK = 1<<WEST;
    constexpr int EAST_MASK = 1<<EAST;
    constexpr int ALL_FACES = 63;
    
    inline unsigned int bitmask(int face) {
        return 1 << face;
    }
    inline unsigned int bitmask(int face, bool val) {
        return (val?1:0) << face;
    }
    
    inline int oppositeFace(int face) {
        return face ^ 1;
    }
    
    inline bool hasFace(unsigned int faces, int face) {
        unsigned int mask = bitmask(face);
        return !!(mask & faces);
    }
    
    int rotateFace(int face, int rotation);
    int rotateFaces(int mask, int rotation);
    
    int faceFromName(const char *name);
    
    extern const glm::vec3 vector[];
    extern const int int_vector[6][3];
}

#endif
