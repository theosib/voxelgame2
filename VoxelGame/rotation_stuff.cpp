#include <glm/gtx/string_cast.hpp>
#include <glm/gtc/matrix_access.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <iomanip>
#include <vector>

glm::mat4 xrm[4], yrm[4], zrm[4];

void compute_rm(glm::mat4 *arr, int angle, int axis)
{
    glm::vec3 ax(0,0,0);
    ax[axis] = 1;
    
    float an = glm::radians(angle*90.0f);
    
    glm::mat4 rot(1.0f);
    rot = glm::translate(glm::mat4(1.0f), glm::vec3(-0.5f, -0.5f, -0.5f)) * rot;
    rot = glm::rotate(glm::mat4(1.0f), an, ax) * rot;
    rot = glm::translate(glm::mat4(1.0f), glm::vec3(0.5f, 0.5f, 0.5f)) * rot;
    
    float *p = glm::value_ptr(rot);
    for (int i=0; i<16; i++) {
        p[i] = round(p[i]);
    }
    
    arr[angle] = rot;
}

void compute_all_rm()
{
    for (int angle=0; angle<4; angle++) {
        compute_rm(xrm, angle, 0);
        compute_rm(yrm, angle, 1);
        compute_rm(zrm, angle, 2);
    }
}

glm::mat4 get_mat(int axis, int angle)
{
    switch (axis) {
    case 0:
        return xrm[angle];
    case 1:
        return yrm[angle];
    case 2:
        return zrm[angle];
    }
    return glm::mat4(1.0f);
}

int perms[6][3] = {
    0, 1, 2,
    0, 2, 1,
    1, 0, 2,
    1, 2, 0,
    2, 0, 1,
    2, 1, 0
};


struct Angler {
    int xr, yr, zr, ix;
    glm::mat4 m;
    
    bool operator<(const Angler& o) const {
        // if (xr < o.xr) return true;
        // if (xr > o.xr) return false;
        // if (yr < o.yr) return true;
        // if (yr > o.yr) return false;
        // if (zr < o.zr) return true;
        // if (zr > o.zr) return false;
        // return false;
        int cnt = (!!xr) + (!!yr) + (!!zr);
        int ocnt = (!!o.xr) + (!!o.yr) + (!!o.zr);
        if (cnt < ocnt) return true;
        if (cnt > ocnt) return false;
        
        int order = (!!xr) + ((!!yr)<<1) + ((!!zr)<<2);
        int oorder = (!!o.xr) + ((!!o.yr)<<1) + ((!!o.zr)<<2);
        if (order < oorder) return true;
        if (order > oorder) return false;
        
        int tot = xr+yr+zr;
        int otot = o.xr + o.yr + o.zr;
        if (tot < otot) return true;
        if (tot > otot) return false;
        return ix < o.ix;
    }
    
    bool operator==(const Angler& o) {
        return xr == o.xr && yr == o.yr && zr == o.zr;
    }
};

Angler compute_rot(int i)
{
    Angler a;
    // int angle0 = i & 3;
    // int angle1 = (i>>2) & 3;
    // int angle2 = (i>>4) & 3;
    int angle0 = (i>>4) & 3;
    int angle1 = i & 3;
    int angle2 = (i>>2) & 3;
    int perm = i >> 6;
    glm::mat4 r(1.0f);
    glm::mat4 r1 = get_mat(perms[perm][0], angle0);
    glm::mat4 r2 = get_mat(perms[perm][1], angle1);
    glm::mat4 r3 = get_mat(perms[perm][2], angle2);
    a.m = r3 * r2 * r1 * r;
    *(&a.xr + perms[perm][0]) = angle0;
    *(&a.xr + perms[perm][1]) = angle1;
    *(&a.xr + perms[perm][2]) = angle2;
    a.ix = i;
    return a;
}

void print_rot(int i) 
{
    // int i_angle0 = i & 3;
    // int i_angle1 = (i>>2) & 3;
    // int i_angle2 = (i>>4) & 3;
    int i_angle0 = (i>>4) & 3;
    int i_angle1 = i & 3;
    int i_angle2 = (i>>2) & 3;
    int i_perm = i >> 6;        

    std::cout << "    // r" << i;        
    std::cout << " " << (char)('x' + perms[i_perm][0]) << i_angle0;
    std::cout << " " << (char)('x' + perms[i_perm][1]) << i_angle1;
    std::cout << " " << (char)('x' + perms[i_perm][2]) << i_angle2;
}

std::vector<Angler> rots;

int find(const Angler& o)
{
    for (int i=0; i<rots.size(); i++) {
        if (o.m == rots[i].m) return i;
    }
    return -1;
}

void all_rots()
{
    for (int i=0; i<6*4*4*4; i++) {
        rots.push_back(compute_rot(i));
    }
    
    // std::sort(rots.begin(), rots.end());
    
    for (int i=0; i<rots.size(); i++) {
        int j = find(rots[i]);
        if (j >= 0 && j != i) {
            // print_rot(rots[i].ix);
            // std::cout << " same=" << rots[j].ix;
            // std::cout << "\n";
        } else {
            // std::cout << std::endl;
            std::cout << "    {";
            float *p = glm::value_ptr(rots[i].m);
            for (int i=0; i<16; i++) {
                std::cout << std::setw(2) << (int)p[i] << ", ";
            }
            std::cout << "},  ";
            print_rot(rots[i].ix);
            std::cout << "\n";
        }
    }
}

const float rotation_matrices[24][16] = {
    { 1,  0,  0,  0,  0,  1,  0,  0,  0,  0,  1,  0,  0,  0,  0,  1, },      // r0 x0 y0 z0
    { 0,  0, -1,  0,  0,  1,  0,  0,  1,  0,  0,  0,  0,  0,  1,  1, },      // r1 x0 y1 z0
    {-1,  0,  0,  0,  0,  1,  0,  0,  0,  0, -1,  0,  1,  0,  1,  1, },      // r2 x0 y2 z0
    { 0,  0,  1,  0,  0,  1,  0,  0, -1,  0,  0,  0,  1,  0,  0,  1, },      // r3 x0 y3 z0
    { 0,  1,  0,  0, -1,  0,  0,  0,  0,  0,  1,  0,  1,  0,  0,  1, },      // r4 x0 y0 z1
    { 0,  0, -1,  0, -1,  0,  0,  0,  0,  1,  0,  0,  1,  0,  1,  1, },      // r5 x0 y1 z1
    { 0, -1,  0,  0, -1,  0,  0,  0,  0,  0, -1,  0,  1,  1,  1,  1, },      // r6 x0 y2 z1
    { 0,  0,  1,  0, -1,  0,  0,  0,  0, -1,  0,  0,  1,  1,  0,  1, },      // r7 x0 y3 z1
    {-1,  0,  0,  0,  0, -1,  0,  0,  0,  0,  1,  0,  1,  1,  0,  1, },      // r8 x0 y0 z2
    { 0,  0, -1,  0,  0, -1,  0,  0, -1,  0,  0,  0,  1,  1,  1,  1, },      // r9 x0 y1 z2
    { 1,  0,  0,  0,  0, -1,  0,  0,  0,  0, -1,  0,  0,  1,  1,  1, },      // r10 x0 y2 z2
    { 0,  0,  1,  0,  0, -1,  0,  0,  1,  0,  0,  0,  0,  1,  0,  1, },      // r11 x0 y3 z2
    { 0, -1,  0,  0,  1,  0,  0,  0,  0,  0,  1,  0,  0,  1,  0,  1, },      // r12 x0 y0 z3
    { 0,  0, -1,  0,  1,  0,  0,  0,  0, -1,  0,  0,  0,  1,  1,  1, },      // r13 x0 y1 z3
    { 0,  1,  0,  0,  1,  0,  0,  0,  0,  0, -1,  0,  0,  0,  1,  1, },      // r14 x0 y2 z3
    { 0,  0,  1,  0,  1,  0,  0,  0,  0,  1,  0,  0,  0,  0,  0,  1, },      // r15 x0 y3 z3
    { 1,  0,  0,  0,  0,  0,  1,  0,  0, -1,  0,  0,  0,  1,  0,  1, },      // r16 x1 y0 z0
    { 0,  1,  0,  0,  0,  0,  1,  0,  1,  0,  0,  0,  0,  0,  0,  1, },      // r20 x1 y0 z1
    {-1,  0,  0,  0,  0,  0,  1,  0,  0,  1,  0,  0,  1,  0,  0,  1, },      // r24 x1 y0 z2
    { 0, -1,  0,  0,  0,  0,  1,  0, -1,  0,  0,  0,  1,  1,  0,  1, },      // r28 x1 y0 z3
    {-1,  0,  0,  0,  0,  0, -1,  0,  0, -1,  0,  0,  1,  1,  1,  1, },      // r18 x1 y2 z0
    { 0, -1,  0,  0,  0,  0, -1,  0,  1,  0,  0,  0,  0,  1,  1,  1, },      // r22 x1 y2 z1
    { 1,  0,  0,  0,  0,  0, -1,  0,  0,  1,  0,  0,  0,  0,  1,  1, },      // r26 x1 y2 z2
    { 0,  1,  0,  0,  0,  0, -1,  0, -1,  0,  0,  0,  1,  0,  1,  1, },      // r30 x1 y2 z3
};

const float rotation_faces[6][3] = {
    { 0, -1, 0 },
    { 0, 1, 0 },
    { 0, 0, -1 },
    { 0, 0, 1 },
    { -1, 0, 0 },
    { 1, 0, 0 }
};

int find_face(glm::vec3 f)
{
    for (int i=0; i<6; i++) {
        glm::vec3 g = glm::make_vec3(rotation_faces[i]);
        if (g == f) return i;
    }
    return -1;
}

int test_face(int rotation_i, int face_i)
{
    glm::mat4 rm = glm::inverse(glm::make_mat4(rotation_matrices[rotation_i]));
    glm::vec3 fv3 = glm::make_vec3(rotation_faces[face_i]);
    glm::vec4 fv = glm::vec4(fv3, 0);
    glm::vec4 fvr = rm * fv;
    glm::vec3 fvr3 = glm::vec3(fvr);
    return find_face(fvr3);
}

void test_faces(int rotation_i)
{
    std::cout << "{";
    for (int f=0; f<6; f++) {
        int g = test_face(rotation_i, f);
        std::cout << g << ", ";
    }
    std::cout << "},\n";
}

float cube_faces[24][3] = {
// face:bottom:
    0,  0,  1, 
    0,  0,  0, 
    1,  0,  0, 
    1,  0,  1, 
// face:top:
    0,  1,  0, 
    0,  1,  1, 
    1,  1,  1, 
    1,  1,  0, 
// face:north:
    1,  1,  0, 
    1,  0,  0, 
    0,  0,  0, 
    0,  1,  0,  
// face:south:
    0,  1,  1, 
    0,  0,  1, 
    1,  0,  1, 
    1,  1,  1, 
// face:west:
    0,  1,  0, 
    0,  0,  0, 
    0,  0,  1, 
    0,  1,  1, 
// face:east:
    1,  1,  1, 
    1,  0,  1, 
    1,  0,  0, 
    1,  1,  0, 
};

void rotate_point_45(glm::vec3 p)
{
    p.x -= 0.5;
    p.x *= sqrtf(2.0f);
    p.z -= 0.5;
    // p.z *= sqrt(2.0);
    glm::vec4 q(p, 1.0f);
    glm::mat4 r(glm::rotate(glm::mat4(1.0f), glm::radians(-45.0f), glm::vec3(0.0f, 1.0f, 0.0f)));
    q = r * q;
    q.x += 0.5;
    q.z += 0.5;
    std::cout << "    " << q.x << " " << q.y << " " << q.z << std::endl;
}

void rotate_all_45()
{
    for (int i=0; i<24; i++) {
        glm::vec3 p = glm::make_vec3(cube_faces[i]);
        rotate_point_45(p);
    }
}

bool compare_rotation(glm::mat4 m, const float *q)
{
    float *p = glm::value_ptr(m);
    for (int i=0; i<16; i++) {
        if (p[i] != q[i]) return false;
    }
    return true;
}

int find_rotation(glm::mat4 m)
{
    for (int r=0; r<24; r++) {
        if (compare_rotation(m, rotation_matrices[r])) return r;
    }
    return -1;
}

glm::mat4 rotate_identity(glm::vec3 axis, float angle_deg)
{
    float an = glm::radians(angle_deg);
    glm::vec3 ax = glm::normalize(axis);
    
    glm::mat4 rot(1.0f);
    rot = glm::translate(glm::mat4(1.0f), glm::vec3(-0.5f, -0.5f, -0.5f)) * rot;
    rot = glm::rotate(glm::mat4(1.0f), an, ax) * rot;
    rot = glm::translate(glm::mat4(1.0f), glm::vec3(0.5f, 0.5f, 0.5f)) * rot;
    
    float *p = glm::value_ptr(rot);
    for (int i=0; i<16; i++) {
        p[i] = round(p[i] * 4096.0) / 4096.0;
    }
    
    return rot;
}

const float axes[][3] = {
    1, 0, 0,
    0, 1, 0,
    0, 0, 1,
    
    1, 1, 0,
    0, 1, 1,
    1, -1, 0,
    0, -1, 1,
    1, 0, 1,
    -1, 0, 1,
    
    1, 1, 1,
    -1, -1, 1,
    1, -1, -1,
    1, -1, 1,
};

void try_symmetries()
{
    for (int ai=0; ai<13; ai++) {
        glm::vec3 axis = glm::make_vec3(axes[ai]);
        for (int angle=30; angle<360; angle+=30) {
            glm::mat4 rot = rotate_identity(axis, angle);
            int r = find_rotation(rot);
            if (r >= 0) {
                std::cout << "axis=" << glm::to_string(axis) << " angle=" << angle << " rnum=" << r << std::endl;
            }
        }
    }
}

void rotation_test()
{
    // rotate_all_45();
    try_symmetries();
    exit(0);
    
    // compute_all_rm();
    // all_rots();
    for (int r=0; r<24; r++) {
        test_faces(r);
    }
}