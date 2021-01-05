#ifndef _INCLUDED_MESH_HPP
#define _INCLUDED_MESH_HPP

#include <vector>
#include <mutex>
#include <condition_variable>
#include <thread>
#include "texture.hpp"
#include "shader.hpp"
#include "position.hpp"

class CameraModel;

struct RenderData {
    std::vector<float> vertices, texcoords, normals;
    int total_vertices;
    
    void clear();
    
    RenderData() : total_vertices(0) {}
};

struct RenderBuffer {
    int attribute_number;
    unsigned int VBO;
    int num_components;
    RenderBuffer() : VBO(0) {}
    ~RenderBuffer() { deallocate(); }
    
    RenderBuffer(int an, int nc) : attribute_number(an), num_components(nc), VBO(0) {}
    void load(unsigned int VAO, const std::vector<float>& list);
    void deallocate();
};

class Renderer {
private:
    unsigned int VAO;
    Texture *tex;
    // Shader *shader;
    RenderData data;
    RenderBuffer vertex_buffer, texcoord_buffer, normals_buffer;
    BlockPos center;
    bool needs_load;
    
public:
    glm::dvec3 position; // XXX used by translucent objects for sorting
    // glm::dvec3 pos, vel, acc;
    // glm::dvec3 current_pos, target_pos;
    // double current_time, target_time;
    
public:
    Renderer(Texture *t) : tex(t), VAO(0), needs_load(false),
        vertex_buffer(0, 3), texcoord_buffer(2, 2), normals_buffer(1, 3) {}
    ~Renderer() { deallocate(); }
    void deallocate();
    
    // void clearData() { data.clear(); }
    void load_buffers();
    void draw(Shader *shader);
    
    
    void setCenter(const BlockPos& c) { center = c; }
    const BlockPos& getCenter() { return center; }
    Texture* getTexture() { return tex; }
    void setTexture(Texture *t) { tex = t; }
    RenderData *getData() { return &data; }
    void setNeedsLoad() { needs_load = true; }
    // Shader* getShader() { return shader; }
};

class RenderManager {
public:
    static RenderManager instance;
    
    void launch_threads();
    
private:    
    CameraModel *camera;
    glm::dvec3 camera_pos;
    glm::mat4 projection_matrix;
    
    std::mutex              camera_mutex;
    
    std::mutex              chunks_mutex;
    std::condition_variable chunks_condition;
    std::thread             *chunks_thread;
    bool chunks_needs_compute;
    volatile bool chunks_thread_alive;
    
    std::mutex              entities_mutex;
    std::condition_variable entities_condition;
    std::thread             *entities_thread;
    bool entities_needs_compute;
    volatile bool entities_thread_alive;
    
    std::vector<Renderer*> dead_renderers;
    std::mutex             dead_renderer_mutex;
    
    void chunks_thread_loop();
    void entities_thread_loop();
    
    void computeChunkRenders(const glm::dvec3& cp, const glm::mat4& projection, CameraModel *cm);
    void computeEntityRenders(const glm::dvec3& cp, const glm::mat4& projection, CameraModel *cm);
    
public:
    RenderManager();
    ~RenderManager();
    
    void setProjection(const glm::mat4& proj) {
        std::unique_lock<std::mutex> lock(camera_mutex);
        projection_matrix = proj;
    }
    void setCamera(CameraModel *model);
    
    void signalComputeRenders();
        
    void stop();
    
    
    void queueDeleteRenderer(const std::vector<Renderer*>& r);
    void deleteDeadRendererQueue();
};


#endif