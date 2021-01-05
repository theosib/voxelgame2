#ifndef INCLUDED_WORLD_VIEW_HPP
#define INCLUDED_WORLD_VIEW_HPP

#include "chunkview.hpp"
// #include "longconcurrentmap.hpp"
#include "cameracontroller.hpp"

class ChunkView;

class WorldView {
public:
    static WorldView instance;
    
private:
    Shader blockShader;
    Shader entityShader;
    Shader placementShader;
    std::shared_ptr<Renderer> placeblock_render;
    
public:
    WorldView() : entityShader("vertex_entity.glsl", "fragment_entity.glsl"), blockShader("vertex_block.glsl", "fragment_block.glsl"),
          placementShader("vertex_placement.glsl", "fragment_placement.glsl") {
        placeblock_render.reset(new Renderer(0));
    }
    
    void computeChunkRenders(const glm::dvec3& cp, const glm::mat4& projection, CameraModel *camera);
    void computeEntityRenders(const glm::dvec3& cp, const glm::mat4& projection, CameraModel *camera);
    void draw(CameraModel *camera);
    void drawTrans(CameraModel *camera, const std::vector<Chunk *>& chunks);
    
    void drawOneBlock(CameraModel *camera, const BlockPos& pos, const std::string& block, int rotation);
        
    static int outsideFrustum(const glm::mat4& transform, const glm::dvec3& point_in);    
    static int outsideFrustum(const glm::mat4& transform, const BlockPos& adjusted_position) {
        glm::dvec3 point(adjusted_position.X, adjusted_position.Y, adjusted_position.Z);
        return WorldView::outsideFrustum(transform, point);
    }
    
    void setProjection(const glm::mat4& matrix) {
        blockShader.setMat4("projection", matrix);
        entityShader.setMat4("projection", matrix);
        placementShader.setMat4("projection", matrix);
    }
};

#endif