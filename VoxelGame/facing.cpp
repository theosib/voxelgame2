

#include "facing.hpp"
#include <ctype.h>
#include <stdlib.h>

// enum direction {
//     DOWN,   // negative Y
//     UP,     // positive Y
//     NORTH,  // negative Z
//     SOUTH,  // positive Z
//     WEST,   // negative X
//     EAST    // positive X
// };

namespace facing {
    const char *face_name[] = {
        "DOWN",
        "UP",
        "NORTH",
        "SOUTH",
        "WEST",
        "EAST"
    };    
    
    const glm::vec3 float_vector[] = {
        glm::vec3(0.0, -1.0, 0.0),  // DOWN
        glm::vec3(0.0, 1.0, 0.0),   // UP
        glm::vec3(0.0, 0.0, -1.0),  // NORTH
        glm::vec3(0.0, 0.0, 1.0),   // SOUTH
        glm::vec3(-1.0, 0.0, 0.0),  // WEST
        glm::vec3(1.0, 0.0, 0.0)    // EAST
    };
    
    const int int_vector[6][3] {
        {0, -1, 0},
        {0, 1, 0},
        {0, 0, -1},
        {0, 0, 1},
        {-1, 0, 0},
        {1, 0, 0}
    };
    
    int faceFromName(const char *name)
    {
        if (isdigit(*name)) {
            return atoi(name);
        }
        switch (tolower(name[0])) {
        case 'd':
        case 'b':
            return DOWN;
        case 'u':
        case 't':
            return UP;
        case 'n':
            return NORTH;
        case 's':
            return SOUTH;
        case 'w':
            return WEST;
        case 'e':
            return EAST;
        default:
            return -1;
        }
    }
    
    const int faceRotation[24][6] = {
#if 0
        {0, 1, 2, 3, 4, 5, },
        {0, 1, 4, 5, 3, 2, },
        {0, 1, 3, 2, 5, 4, },
        {0, 1, 5, 4, 2, 3, },
        {5, 4, 2, 3, 0, 1, },
        {5, 4, 0, 1, 3, 2, },
        {5, 4, 3, 2, 1, 0, },
        {5, 4, 1, 0, 2, 3, },
        {1, 0, 2, 3, 5, 4, },
        {1, 0, 5, 4, 3, 2, },
        {1, 0, 3, 2, 4, 5, },
        {1, 0, 4, 5, 2, 3, },
        {4, 5, 2, 3, 1, 0, },
        {4, 5, 1, 0, 3, 2, },
        {4, 5, 3, 2, 0, 1, },
        {4, 5, 0, 1, 2, 3, },
        {2, 3, 1, 0, 4, 5, },
        {2, 3, 4, 5, 0, 1, },
        {2, 3, 0, 1, 5, 4, },
        {2, 3, 5, 4, 1, 0, },
        {3, 2, 1, 0, 5, 4, },
        {3, 2, 4, 5, 1, 0, },
        {3, 2, 0, 1, 4, 5, },
        {3, 2, 5, 4, 0, 1, },
#endif
        
        {0, 1, 2, 3, 4, 5, },
        {0, 1, 5, 4, 2, 3, },
        {0, 1, 3, 2, 5, 4, },
        {0, 1, 4, 5, 3, 2, },
        {4, 5, 2, 3, 1, 0, },
        {2, 3, 5, 4, 1, 0, },
        {5, 4, 3, 2, 1, 0, },
        {3, 2, 4, 5, 1, 0, },
        {1, 0, 2, 3, 5, 4, },
        {1, 0, 5, 4, 3, 2, },
        {1, 0, 3, 2, 4, 5, },
        {1, 0, 4, 5, 2, 3, },
        {5, 4, 2, 3, 0, 1, },
        {3, 2, 5, 4, 0, 1, },
        {4, 5, 3, 2, 0, 1, },
        {2, 3, 4, 5, 0, 1, },
        {3, 2, 0, 1, 4, 5, },
        {4, 5, 0, 1, 2, 3, },
        {2, 3, 0, 1, 5, 4, },
        {5, 4, 0, 1, 3, 2, },
        {3, 2, 1, 0, 5, 4, },
        {5, 4, 1, 0, 2, 3, },
        {2, 3, 1, 0, 4, 5, },
        {4, 5, 1, 0, 3, 2, },
        
    };
    
    int rotateFace(int face, int rotation)
    {
        return faceRotation[rotation][face];
    }
    
    int rotateFaces(int mask, int rotation)
    {
        int out = mask & ~ALL_FACES;
        for (int i=0; i<6; i++) {
            if (hasFace(mask, i)) {
                int j = rotateFace(i, rotation);
                out |= 1<<j;
            }
        }
        return out;
    }
    
}
