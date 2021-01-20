#include "mesh.hpp"
#include <glm/gtc/type_ptr.hpp>

void Face::computeNormal()
{
    glm::vec3 a, b, c;
    if (num_vertices==3) {
        a = vertices[0];
        b = vertices[1];
        c = vertices[2];
    } else {
        if (vertices[0] == vertices[1] || vertices[1] == vertices[2]) {
            a = vertices[0];
            b = vertices[2];
            c = vertices[3];
        } else {
            a = vertices[0];
            b = vertices[1];
            c = vertices[2];
        }
    }
    // glm::vec3& a(vertices[0]);
    // glm::vec3& b(vertices[1]);
    // glm::vec3& c(vertices[2]);
    glm::vec3 cb = c - b;
    glm::vec3 ab = a - b;
    glm::vec3 x = glm::cross(cb, ab);
    float mag = glm::length(x);
    x /= mag;
    normal = x;
}

static const unsigned int face_indices[] = {
    0, 1, 2, 0, 2, 3
};

// void Face::getTriangleVertices(std::vector<float>& vertices_out, const glm::dvec3& pos, const BlockPos& center)
void Face::getTriangleVertices(std::vector<float>& vertices_out, float offsetX, float offsetY, float offsetZ)
{
    // int offsetX = pos.X - center.X;
    // int offsetY = pos.Y - center.Y;
    // int offsetZ = pos.Z - center.Z;
    
    int loops = numTriangleVertices();
    for (int i=0; i<loops; i++) {
        int v = face_indices[i];
        vertices_out.push_back(vertices[v].x + offsetX);
        vertices_out.push_back(vertices[v].y + offsetY);
        vertices_out.push_back(vertices[v].z + offsetZ);
    }
}

void Face::getTriangleVertices(std::vector<float>& vertices_out, const glm::mat4& rotation, float offsetX, float offsetY, float offsetZ)
{
    int loops = numTriangleVertices();
    for (int i=0; i<loops; i++) {
        int v = face_indices[i];
        glm::vec3 rv = rotation * glm::vec4(vertices[v], 1.0f);
        vertices_out.push_back(rv.x + offsetX);
        vertices_out.push_back(rv.y + offsetY);
        vertices_out.push_back(rv.z + offsetZ);
    }
}

void Face::getTriangleTextcoords(std::vector<float>& texcoords_out)
{
    int loops = numTriangleVertices();
    for (int i=0; i<loops; i++) {
        int v = face_indices[i];
        texcoords_out.push_back(texcoords[v].x);
        texcoords_out.push_back(texcoords[v].y);
    }
}

void Face::getTriangleNormals(std::vector<float>& normals_out)
{
    int loops = numTriangleVertices();
    for (int i=0; i<loops; i++) {
        normals_out.push_back(normal.x);
        normals_out.push_back(normal.y);
        normals_out.push_back(normal.z);
    }
}

void Face::getTriangleNormals(std::vector<float>& normals_out, const glm::mat4& rotation)
{
    int loops = numTriangleVertices();
    glm::vec3 rn = rotation * glm::vec4(normal, 0.0f);
    for (int i=0; i<loops; i++) {
        normals_out.push_back(rn.x);
        normals_out.push_back(rn.y);
        normals_out.push_back(rn.z);
    }
}


int Mesh::numTriangleVertices(int show_faces)
{
    int total = 0;
    for (int face=0; face<faces.size(); face++) {
        if (face<facing::NUM_FACES && !facing::hasFace(show_faces, face)) continue;
        total += faces[face].numTriangleVertices();
    }
    return total;
}

void Mesh::getTriangleVertices(int show_faces, std::vector<float>& vertices_out, const BlockPos& pos, const BlockPos& center)
{
    glm::dvec3 pos2(pos.X, pos.Y, pos.Z);
    getTriangleVertices(show_faces, vertices_out, pos2, center);
}

void Mesh::getTriangleVertices(int show_faces, std::vector<float>& vertices_out, const glm::dvec3& pos, const BlockPos& center)
{
    double offsetX = pos.x - center.X;
    double offsetY = pos.y - center.Y;
    double offsetZ = pos.z - center.Z;
    
    for (int face=0; face<faces.size(); face++) {
        if (face<facing::NUM_FACES && !facing::hasFace(show_faces, face)) continue;
        faces[face].getTriangleVertices(vertices_out, (float)offsetX, (float)offsetY, (float)offsetZ);
    }
}

void Mesh::getTriangleVertices(int show_faces, std::vector<float>& vertices_out, const glm::mat4& rotation, const BlockPos& pos, const BlockPos& center)
{
    glm::dvec3 pos2(pos.X, pos.Y, pos.Z);
    getTriangleVertices(show_faces, vertices_out, rotation, pos2, center);
}

void Mesh::getTriangleVertices(int show_faces, std::vector<float>& vertices_out, const glm::mat4& rotation, const glm::dvec3& pos, const BlockPos& center)
{
    double offsetX = pos.x - center.X;
    double offsetY = pos.y - center.Y;
    double offsetZ = pos.z - center.Z;
    
    for (int face=0; face<faces.size(); face++) {
        if (face<facing::NUM_FACES && !facing::hasFace(show_faces, face)) continue;
        faces[face].getTriangleVertices(vertices_out, rotation, (float)offsetX, (float)offsetY, (float)offsetZ);
    }
}


void Mesh::getTriangleTexCoords(int show_faces, std::vector<float>& texcoords_out)
{
    for (int face=0; face<faces.size(); face++) {
        if (face<facing::NUM_FACES && !facing::hasFace(show_faces, face)) continue;
        faces[face].getTriangleTextcoords(texcoords_out);
    }
}

void Mesh::getTriangleNormals(int show_faces, std::vector<float>& normals_out)
{
    for (int face=0; face<faces.size(); face++) {
        if (face<facing::NUM_FACES && !facing::hasFace(show_faces, face)) continue;
        faces[face].getTriangleNormals(normals_out);
    }
}

void Mesh::getTriangleNormals(int show_faces, std::vector<float>& normals_out, const glm::mat4& rotation)
{
    for (int face=0; face<faces.size(); face++) {
        if (face<facing::NUM_FACES && !facing::hasFace(show_faces, face)) continue;
        faces[face].getTriangleNormals(normals_out, rotation);
    }
}

void Mesh::getCollision(std::vector<geom::Box>& boxes, double offsetX, double offsetY, double offsetZ, int rotation)
{
    glm::mat4 rot = getRotationMatrix(rotation);
    for (auto i=collision.begin(); i!=collision.end(); ++i) {
        boxes.push_back(i->rotate(rot).offset(offsetX, offsetY, offsetZ));
    }
}

const float Mesh::rotation_matrices[24][16] = {
    { 1,  0,  0,  0,  0,  1,  0,  0,  0,  0,  1,  0,  0,  0,  0,  1, },      // r0  x0 y0 z0    none
    { 0,  0, -1,  0,  0,  1,  0,  0,  1,  0,  0,  0,  0,  0,  1,  1, },      // r1  x0 y1 z0     90 +Y
    {-1,  0,  0,  0,  0,  1,  0,  0,  0,  0, -1,  0,  1,  0,  1,  1, },      // r2  x0 y2 z0    180 +Y
    { 0,  0,  1,  0,  0,  1,  0,  0, -1,  0,  0,  0,  1,  0,  0,  1, },      // r3  x0 y3 z0    270 +Y
    { 0,  1,  0,  0, -1,  0,  0,  0,  0,  0,  1,  0,  1,  0,  0,  1, },      // r4  x0 y0 z1     90 +Z
    { 0,  0, -1,  0, -1,  0,  0,  0,  0,  1,  0,  0,  1,  0,  1,  1, },      // r5  x0 y1 z1    120 -X+Y+Z  240 +X-Y-Z
    { 0, -1,  0,  0, -1,  0,  0,  0,  0,  0, -1,  0,  1,  1,  1,  1, },      // r6  x0 y2 z1    180 -X+Y    180 +X-Z
    { 0,  0,  1,  0, -1,  0,  0,  0,  0, -1,  0,  0,  1,  1,  0,  1, },      // r7  x0 y3 z1    240 -X+Y-Z  120 +X-Y+Z
    {-1,  0,  0,  0,  0, -1,  0,  0,  0,  0,  1,  0,  1,  1,  0,  1, },      // r8  x0 y0 z2    180 +Z      
    { 0,  0, -1,  0,  0, -1,  0,  0, -1,  0,  0,  0,  1,  1,  1,  1, },      // r9  x0 y1 z2    180 +X-Z    180 -X+Z
    { 1,  0,  0,  0,  0, -1,  0,  0,  0,  0, -1,  0,  0,  1,  1,  1, },      // r10 x0 y2 z2    180 +X      
    { 0,  0,  1,  0,  0, -1,  0,  0,  1,  0,  0,  0,  0,  1,  0,  1, },      // r11 x0 y3 z2    180 +X+Z    
    { 0, -1,  0,  0,  1,  0,  0,  0,  0,  0,  1,  0,  0,  1,  0,  1, },      // r12 x0 y0 z3    270 +Z
    { 0,  0, -1,  0,  1,  0,  0,  0,  0, -1,  0,  0,  0,  1,  1,  1, },      // r13 x0 y1 z3    120 +X+Y-Z  240 -X-Y+Z
    { 0,  1,  0,  0,  1,  0,  0,  0,  0,  0, -1,  0,  0,  0,  1,  1, },      // r14 x0 y2 z3    180 +X+Y    
    { 0,  0,  1,  0,  1,  0,  0,  0,  0,  1,  0,  0,  0,  0,  0,  1, },      // r15 x0 y3 z3    240 +X+Y+Z
    { 1,  0,  0,  0,  0,  0,  1,  0,  0, -1,  0,  0,  0,  1,  0,  1, },      // r16 x1 y0 z0     90 +X
    { 0,  1,  0,  0,  0,  0,  1,  0,  1,  0,  0,  0,  0,  0,  0,  1, },      // r20 x1 y0 z1    120 +X+Y+Z
    {-1,  0,  0,  0,  0,  0,  1,  0,  0,  1,  0,  0,  1,  0,  0,  1, },      // r24 x1 y0 z2    180 +Y+Z
    { 0, -1,  0,  0,  0,  0,  1,  0, -1,  0,  0,  0,  1,  1,  0,  1, },      // r28 x1 y0 z3    240 -Z+Y+Z  120 +X-Y-Z
    {-1,  0,  0,  0,  0,  0, -1,  0,  0, -1,  0,  0,  1,  1,  1,  1, },      // r18 x1 y2 z0    180 +Y-Z    180 -Y+Z
    { 0, -1,  0,  0,  0,  0, -1,  0,  1,  0,  0,  0,  0,  1,  1,  1, },      // r22 x1 y2 z1    120 -X+Y-Z  
    { 1,  0,  0,  0,  0,  0, -1,  0,  0,  1,  0,  0,  0,  0,  1,  1, },      // r26 x1 y2 z2    270 +X
    { 0,  1,  0,  0,  0,  0, -1,  0, -1,  0,  0,  0,  1,  0,  1,  1, },      // r30 x1 y2 z3    240 +X+Y-Z
};

// const char Mesh::rotation_faces[24][6] = {
//     {0, 1, 2, 3, 4, 5},
//
// };

glm::mat4 Mesh::getRotationMatrix(int rotation_num)
{
    return glm::make_mat4(rotation_matrices[rotation_num]);
}
