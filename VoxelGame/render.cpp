#include "gl_includes.hpp"
#include "render.hpp"
#include "world.hpp"
#include "compat.hpp"
#include "worldview.hpp"
#include "cameramodel.hpp"
#include <iostream>
#include "time.hpp"

void RenderBuffer::deallocate()
{
    // std::cout << "Delete VBO=" << VBO << std::endl;
    if (VBO) {
        glDeleteBuffers(1, &VBO);
        VBO = 0;
    }
}

void Renderer::deallocate()
{
    // std::cout << "Delete VAO=" << VAO << std::endl;
    if (VAO) {
        glDeleteVertexArrays(1, &VAO);
        VAO = 0;
    }
}

void RenderData::clear()
{
    vertices.clear();
    texcoords.clear();
    normals.clear();
    total_vertices = 0;
}

void RenderBuffer::load(unsigned int VAO, const std::vector<float>& list)
{
    glBindVertexArray(VAO);
    if (!VBO) glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, list.size() * sizeof(float), list.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(attribute_number, num_components, GL_FLOAT, GL_FALSE, 0, (void*)0); // Actually bind the VBO to the VAO
    glEnableVertexAttribArray(attribute_number);
    glBindVertexArray(0);
    // std::cout << "Load: VAO=" << VAO << " VBO=" << VBO << " listsize=" << list.size() << " attr=" << attribute_number << std::endl;
}

void Renderer::load_buffers()
{
    // __builtin_trap();
    
    if (!needs_load) return;
    needs_load = false;
    
    // std::cout << "Loading buffers" << std::endl;
    if (!VAO) glGenVertexArrays(1, &VAO);
    vertex_buffer.load(VAO, data.vertices);
    texcoord_buffer.load(VAO, data.texcoords);
    normals_buffer.load(VAO, data.normals);
}

void Renderer::draw(Shader *shader)
{
    // std::cout << "draw tv=" << data.total_vertices << " VAO=" << VAO << std::endl;
    glBindVertexArray(VAO);
    shader->use();
    tex->use(0);
    glDrawArrays(GL_TRIANGLES, 0, data.total_vertices);
    glBindVertexArray(0);
}



RenderManager RenderManager::instance;

RenderManager::~RenderManager()
{
    stop();
}

RenderManager::RenderManager()
{
    chunks_thread = 0;
    chunks_needs_compute = false;
    chunks_thread_alive = false;
    entities_thread = 0;
    entities_needs_compute = false;
    entities_thread_alive = false;
}

// static void render_manager_entry(RenderManager *self)
// {
//     self->thread_loop();
// }

void RenderManager::launch_threads()
{
    chunks_thread_alive = true;
    chunks_thread = new std::thread(&RenderManager::chunks_thread_loop, this);
    entities_thread_alive = true;
    entities_thread = new std::thread(&RenderManager::entities_thread_loop, this);
}

void RenderManager::chunks_thread_loop()
{
    glm::dvec3 cp;
    glm::mat4 pm;
    CameraModel *cm;
    
    while (chunks_thread_alive) {
        {
            std::unique_lock<std::mutex> lock(chunks_mutex);
            while (!chunks_needs_compute && chunks_thread_alive) {
                chunks_condition.wait(lock);
            }
            chunks_needs_compute = false;
        }
        if (!chunks_thread_alive) break;
        
        {
            std::unique_lock<std::mutex> lock(camera_mutex);
            cm = camera;
            cp = camera_pos;
            pm = projection_matrix;
        }
        computeChunkRenders(cp, pm, cm);
    }
}

void RenderManager::entities_thread_loop()
{
    glm::dvec3 cp;
    glm::mat4 pm;
    CameraModel *cm;
    
    while (entities_thread_alive) {
        {
            std::unique_lock<std::mutex> lock(entities_mutex);
            while (!entities_needs_compute && entities_thread_alive) {
                entities_condition.wait(lock);
            }
            entities_needs_compute = false;
        }
        if (!entities_thread_alive) break;
        
        {
            std::unique_lock<std::mutex> lock(camera_mutex);
            cm = camera;
            cp = camera_pos;
            pm = projection_matrix;
        }
        computeEntityRenders(cp, pm, cm);
    }
}

void RenderManager::setCamera(CameraModel *model)
{
    std::unique_lock<std::mutex> lock(camera_mutex);
    camera = model;
    camera_pos = camera->getPos();
}


void RenderManager::signalComputeRenders()
{
    {
        std::unique_lock<std::mutex> lock(chunks_mutex);
        chunks_needs_compute = true;
    }
    chunks_condition.notify_one();

    {
        std::unique_lock<std::mutex> lock(entities_mutex);
        entities_needs_compute = true;
    }
    entities_condition.notify_one();
}


void RenderManager::computeChunkRenders(const glm::dvec3& cp, const glm::mat4& projection, CameraModel *cm)
{
    WorldView::instance.computeChunkRenders(cp, projection, cm);
}

void RenderManager::computeEntityRenders(const glm::dvec3& cp, const glm::mat4& projection, CameraModel *cm)
{
    WorldView::instance.computeEntityRenders(cp, projection, cm);
}

void RenderManager::stop()
{
    if (chunks_thread) {
        chunks_thread_alive = false;
        chunks_condition.notify_one();
        chunks_thread->join();
        delete chunks_thread;
        chunks_thread = 0;
    }    
    if (entities_thread) {
        entities_thread_alive = false;
        entities_condition.notify_one();
        entities_thread->join();
        delete entities_thread;
        entities_thread = 0;
    }    
}

void RenderManager::deleteDeadRendererQueue()
{
    std::unique_lock<std::mutex> lock(dead_renderer_mutex);
    for (auto i=dead_renderers.begin(); i!=dead_renderers.end(); ++i) {
        delete *i;
    }
    dead_renderers.clear();
}

void RenderManager::queueDeleteRenderer(const std::vector<Renderer*>& r) {
    std::unique_lock<std::mutex> lock(dead_renderer_mutex);
    dead_renderers.insert(std::end(dead_renderers), std::begin(r), std::end(r));
}
