#ifndef INCLUDED_UI_ELEMENTS_HPP
#define INCLUDED_UI_ELEMENTS_HPP

#include <glm/glm.hpp>
#include <string>
#include "position.hpp"
#include "shader.hpp"
#include "cameramodel.hpp"
#include "render.hpp"

class UIElements {
public:
    static UIElements instance;
    
private:
    Shader outlineShader;
    unsigned int outline_VAO, outline_VBO;
    
    Shader crossShader;
    unsigned int cross_VAO, cross_VBO;
    
    Shader blocklistShader;
    Renderer *blocklist_render;
    
public:
    UIElements() : outline_VAO(0), outline_VBO(0), cross_VAO(0), cross_VBO(0),
        outlineShader("vertex_outline.glsl", "fragment_outline.glsl"),
        crossShader("vertex_cross.glsl", "fragment_cross.glsl"),
        blocklistShader("vertex_blocklist.glsl", "fragment_blocklist.glsl")
    {
        blocklist_render = new Renderer(0);
    }
    
    ~UIElements() {
        RenderManager::instance.queueDeleteRenderer(blocklist_render);
    }
    
    void drawBlockHighlight(CameraModel *camera, const BlockPos& pos, int show_faces);
    void getHighlightVertices(std::vector<float>& vertices_out, const BlockPos& pos, const BlockPos& center, int face);

    void setHighlightProjection(const glm::mat4& matrix) {
        outlineShader.setMat4("projection", matrix);
    }
    
    void drawCrosshair(int window_width, int window_height, float size);
    
    std::string drawBlockMenu(int window_width, int window_height, double mousex, double mousey);
};




#endif