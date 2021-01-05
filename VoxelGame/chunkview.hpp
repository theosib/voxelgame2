#ifndef INCLUDED_CHUNK_VIEW_HPP
#define INCLUDED_CHUNK_VIEW_HPP

class Chunk;
#include "chunk.hpp"
#include "render.hpp"
#include "position.hpp"

class ChunkView {
    friend class Chunk;
    
private:
    Chunk *chunk;
    
    // Visual update flags
    bool block_visual_modified[sizes::chunk_storage_size];
    bool chunk_visual_modified;
        
    uint8_t block_show_faces[sizes::chunk_storage_size];
    //bool needs_update_render;
    volatile bool render_is_valid;

    glm::mat4 projection;
    // BlockPos center;
    BlockPos draw_frustum_center;
    // XXX need mutex around resizing of these vectors!
    std::vector<Renderer *> render, render_alt;
    
    // Translucent objects: One renderer per block
    std::vector<Renderer *> *trans, *trans_alt;
    
    void setShowFace(int index, int face, bool val) {
        block_show_faces[index] &= ~facing::bitmask(face);
        block_show_faces[index] |= facing::bitmask(face, val);
    }

public:
    ChunkView(Chunk *c);
    ~ChunkView();
    
    void markUpdated(uint16_t index) {
        block_visual_modified[index] = true;
        chunk_visual_modified = true;
    }
    
    void markChunkUpdated();
    
    // Configure the camera position for centering
    // void setCameraPos(const glm::dvec3& camera_pos);
    void setProjection(const glm::mat4& pr) {
        projection = pr;
    }
    bool insideFrustum(const glm::mat4& view, const BlockPos& center);
    
    // Recompute block face visibility
    void updateAllBlockFaces();
    void updateBlockFaces(int index, const BlockPos& pos);
    
    // Methods for render compute thread
    void computeAllRenders(const BlockPos& center);
    void computeTextureRender(int texture_id, const BlockPos& center);
    void renderIterateBlocks(RenderData *render, int texture_id, const BlockPos& center);
    void transIterateBlocks(const BlockPos& center);
    void copyTransRenders(std::vector<Renderer *>& all_trans);
    void computeUpdates(const BlockPos& center, const glm::mat4& projection, const glm::mat4& view);
    
    // Methods for graphics thread
    void draw(Shader *shader, CameraModel *camera);
};

#endif