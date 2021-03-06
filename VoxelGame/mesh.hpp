#ifndef INCLUDED_MESH_HPP
#define INCLUDED_MESH_HPP

#include <memory>
#include <vector>
#include <glm/glm.hpp>
#include "position.hpp"
#include "geometry.hpp"

class Face {
private:
    glm::vec3 vertices[4];
    glm::vec2 texcoords[4];
    int num_vertices;
    int num_texcoords;
    glm::vec3 normal;
    
public:
    Face() : num_texcoords(0), num_vertices(0) {}
        
    // Building a face
    void computeNormal();
    void addVertex(float x, float y, float z) {
        vertices[num_vertices].x = x;
        vertices[num_vertices].y = y;
        vertices[num_vertices].z = z;
        num_vertices++;
        if (num_vertices >= 3) computeNormal();
    }
    void addVertex(const glm::vec3& vertex) {
        vertices[num_vertices++] = vertex;
        if (num_vertices >= 3) computeNormal();
    }
    void addTexCoord(float x, float y) {
        texcoords[num_texcoords].x = x;
        texcoords[num_texcoords].y = y;
        num_texcoords++;
    }
    void addTexCoord(const glm::vec2& texcoord) {
        texcoords[num_texcoords++] = texcoord;
    }
    size_t numVertices() { return num_vertices; }
    glm::vec3& getVertex(int n) { return vertices[n]; }
    glm::vec2& getTexCoord(int n) { return texcoords[n]; }
    
    // Getting a face into a triangle list
    void getTriangleVertices(std::vector<float>& vertices_out, float offsetX, float offsetY, float offsetZ);
    void getTriangleVertices(std::vector<float>& vertices_out, const glm::mat4& rotation, float offsetX, float offsetY, float offsetZ);
    void getTriangleTextcoords(std::vector<float>& texcoords_out);
    void getTriangleNormals(std::vector<float>& normals_out);
    void getTriangleNormals(std::vector<float>& normals_out, const glm::mat4& rotation);
    int numTriangleVertices() {
        if (!numVertices()) return 0;
        return (numVertices()==3) ? 3 : 6;
        // XXX support bigger polygons later?
    }
};

class Mesh;
typedef std::shared_ptr<Mesh> MeshPtr;

class Mesh {
private:
    int texture_index;
    std::vector<Face> faces;
    unsigned char solid_faces;
    std::vector<geom::Box> collision;
    bool translucent;
    
    static const float rotation_matrices[24][16];
    
protected:
    Mesh() : solid_faces(0), translucent(false), texture_index(-1) { faces.resize(facing::NUM_FACES); }
    
public:
    
    // Building a mesh
    void setTexture(int texid) { texture_index = texid; }
    size_t numFaces() const { return faces.size(); }
    Face *getFace(int i) {
        if (i < 0) return 0;
        size_t n = faces.size();
        if (i >= n) faces.resize(i+1);
        return &faces[i]; 
    }
    Face *addFace() { 
        size_t i = faces.size();
        faces.resize(i+1);
        return &faces[i];
    }
    unsigned char getSolidFaces() { return solid_faces; }
    bool faceIsSolid(int face) {
        if (face >= facing::NUM_FACES) return false;
        return facing::hasFace(solid_faces, face);
    }
    void setSolidFaces(int f) { solid_faces = f; }
    
    // Loading mesh from file
    void loadMesh(const std::string& name);
    void addVertices(Face *face, std::vector<float>& values, int key, bool int_tex_coords, int texture_index);
    
    // Getting a mesh into a triangle list
    int getTextureIndex() { return texture_index; }
    int numTriangleVertices(int show_faces);
    void getTriangleVertices(int show_faces, std::vector<float>& vertices_out, const glm::dvec3& pos, const BlockPos& center);
    void getTriangleVertices(int show_faces, std::vector<float>& vertices_out, const BlockPos& pos, const BlockPos& center);
    void getTriangleVertices(int show_faces, std::vector<float>& vertices_out, const glm::mat4& rotation, const glm::dvec3& pos, const BlockPos& center);
    void getTriangleVertices(int show_faces, std::vector<float>& vertices_out, const glm::mat4& rotation, const BlockPos& pos, const BlockPos& center);
    void getTriangleTexCoords(int show_faces, std::vector<float>& texcoords_out);
    void getTriangleNormals(int show_faces, std::vector<float>& normals_out);
    void getTriangleNormals(int show_faces, std::vector<float>& normals_out, const glm::mat4& rotation);
    bool isTranslucent() { return translucent; }
    
    // Create a new mesh
    static MeshPtr makeMesh() {
        return std::shared_ptr<Mesh>(new Mesh);
    }
    
    static glm::mat4 getRotationMatrix(int rotation_num);
    
    void addCollisionBox(const geom::Box& box) {
        collision.push_back(box);
    }
    
    void getCollision(std::vector<geom::Box>& boxes, double offsetX, double offsetY, double offsetZ, int rotation);
};

#endif
