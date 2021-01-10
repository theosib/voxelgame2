#include "chunkview.hpp"
#include "block.hpp"
#include "world.hpp"
#include "compat.hpp"

ChunkView::ChunkView(Chunk *c)
{
    chunk = c;
    memset(block_show_faces, 0, sizeof(block_show_faces));
    markChunkUpdated();
    trans = 0;
    trans_alt = 0;
}

void ChunkView::markChunkUpdated()
{
    memset(block_visual_modified, 1, sizeof(block_visual_modified));
    chunk_visual_modified = true;
}

ChunkView::~ChunkView()
{
    RenderManager::instance.queueDeleteRenderer(render);
    RenderManager::instance.queueDeleteRenderer(render_alt);
    if (trans) {
        RenderManager::instance.queueDeleteRenderer(*trans);
        delete trans;
    }
    if (trans_alt) {
        RenderManager::instance.queueDeleteRenderer(*trans_alt);
        delete trans_alt;
    }

#if 0    
    for (auto i=render.begin(); i!=render.end(); ++i) {
        if (*i) delete *i;
    }
    for (auto i=render_alt.begin(); i!=render_alt.end(); ++i) {
        if (*i) delete *i;
    }
    if (trans) {
        for (auto i=trans->begin(); i!=trans->end(); ++i) {
            if (*i) delete *i;
        }
        delete trans;
    }
    if (trans_alt) {
        for (auto i=trans_alt->begin(); i!=trans_alt->end(); ++i) {
            if (*i) delete *i;
        }
        delete trans_alt;
    }
#endif
}

void ChunkView::updateAllBlockFaces()
{
    // std::cout << "updateAllBlockFaces modified=" << chunk->chunk_modified << "\n";

    for (int i=0; i<sizes::chunk_storage_size; i++) {
        BlockPos bpos = chunk->decodeIndex(i);
        updateBlockFaces(i, bpos);
    }
    
    //needs_update_render = true;
    // std::cout << "updateAllBlockFaces need=" << needs_update_render << std::endl;
}

// setBlock causes neighboring blocks to be marked, so we only have
// to fix up our own face visibility
void ChunkView::updateBlockFaces(int index, const BlockPos& pos)
{
    if (!block_visual_modified[index]) return;
    block_visual_modified[index] = false;

    BlockPtr self_block = chunk->getBlock(pos);
    if (!self_block) return;
    
    // Get the types of neighboring blocks
    BlockPtr neighbor_blocks[facing::NUM_FACES];
    World::instance.getNeighborBlocks(pos, neighbor_blocks, false, World::NoLoad);
    MeshPtr self_mesh = self_block->getMesh();
        
    for (int face=0; face<facing::NUM_FACES; face++) {
        bool visible = true;
        BlockPtr neighbor_block = neighbor_blocks[face];
        if (neighbor_block) {
            MeshPtr neighbor_mesh = neighbor_block->getMesh();
            int opposite_face = facing::oppositeFace(face);
            
            bool self_solid = self_mesh->faceIsSolid(facing::rotateFace(face, self_block->getRotation()));
            bool neighbor_solid = neighbor_mesh->faceIsSolid(facing::rotateFace(opposite_face, neighbor_block->getRotation()));
            
            if (self_solid && neighbor_solid) {
                bool self_trans = self_mesh->isTranslucent();
                bool neighbor_trans = neighbor_mesh->isTranslucent();
                // if (!neighbor_trans) visible = false;
#if 1
                if (self_trans) {
                    if (neighbor_trans) {
                        if (self_mesh == neighbor_mesh) visible = false;
                    } else {
                        visible = false;
                    }
                } else {
                    if (!neighbor_trans) {
                        visible = false;
                    }
                }
#endif
            }
            
            // XXX make this easier to follow
            // if (self_mesh->faceIsSolid(facing::rotateFace(face, self_block->getRotation())) && neighbor_mesh->faceIsSolid(facing::rotateFace(opposite_face, neighbor_block->getRotation()))) visible = false;
        }
        // if (visible) std::cout << "Marking block visible\n";
        setShowFace(index, face, visible);
    }
}

void ChunkView::computeAllRenders(const BlockPos& center)
{
    //if (!needs_update_render) return;
    // std::cout << "computeMeshes need=" << needs_update_mesh << std::endl;
    //needs_update_render = false;
    
    size_t num_tex = TextureLibrary::instance.numTextures();
    if (render.size() < num_tex) {
        render_is_valid = false;
        COMPILER_BARRIER();
        render.resize(num_tex);
    }
    if (render_alt.size() < num_tex) {
        render_is_valid = false;
        COMPILER_BARRIER();
        render_alt.resize(num_tex);
    }
    
    // std::cout << "computeMeshes Num tex: " << num_tex << std::endl;
    draw_frustum_center = center;
    for (int ti=0; ti<num_tex; ti++) {
        computeTextureRender(ti, center);
    }
}

void ChunkView::computeTextureRender(int texture_id, const BlockPos& center)
{
    // std::cout << "Computing render" << std::endl;
    Renderer *mr1 = render_alt[texture_id];
    if (!mr1) mr1 = new Renderer(TextureLibrary::instance.getTexture(texture_id));
    
    // Save center in render for proper alignment when rendering
    mr1->setCenter(center);
    
    renderIterateBlocks(mr1->getData(), texture_id, center);
    mr1->setNeedsLoad();
    
    Renderer *mr2 = render[texture_id];
    if (!mr2) mr2 = new Renderer(TextureLibrary::instance.getTexture(texture_id));
    
    // Swap updated render into place
    render_alt[texture_id] = mr2;
    render[texture_id] = mr1;
}

void ChunkView::renderIterateBlocks(RenderData *render_data, int texture_id, const BlockPos& center)
{
    glm::mat4 rot_matrix;
    
    render_data->clear();
    
    for (int i=0; i<sizes::chunk_storage_size; i++) {
        int block_id = chunk->block_storage[i];
        if (!block_id) continue;
        
        // std::cout << "getting block shape\n";
        MeshPtr mesh = chunk->getMesh(i);
        if (mesh->isTranslucent()) continue; // Skip translucent blocks
        
        int shape_tex_id = mesh->getTextureIndex();
        if (shape_tex_id != texture_id) continue;
        
        BlockPos blockpos = chunk->decodeIndex(i);
        
        int rotation = chunk->getRotation(i);
        int show_faces = facing::rotateFaces(block_show_faces[i], rotation);        
        if (rotation) {
            // std::cout << "rotation\n";
            rot_matrix = Mesh::getRotationMatrix(rotation);
            mesh->getTriangleVertices(show_faces, render_data->vertices, rot_matrix, blockpos, center);
            mesh->getTriangleNormals(show_faces, render_data->normals, rot_matrix);
        } else {
            mesh->getTriangleVertices(show_faces, render_data->vertices, blockpos, center);
            mesh->getTriangleNormals(show_faces, render_data->normals);
        }
        
        mesh->getTriangleTexCoords(show_faces, render_data->texcoords);
        render_data->total_vertices += mesh->numTriangleVertices(show_faces);
        
        // std::cout << "Block at pos " << blockpos.toString() << " vertices:" << mesh_data->total_vertices << " center:" << center.toString() << std::endl;
    }
}

void ChunkView::transIterateBlocks(const BlockPos& center)
{
    glm::mat4 rot_matrix;
    std::vector<Renderer *> *new_trans = new std::vector<Renderer *>();
    
    for (int i=0; i<sizes::chunk_storage_size; i++) {
        int block_id = chunk->block_storage[i];
        if (!block_id) continue;
        
        // std::cout << "getting block shape\n";
        MeshPtr mesh = chunk->getMesh(i);
        if (!mesh->isTranslucent()) continue; // Skip solid blocks
        
        int shape_tex_id = mesh->getTextureIndex();
        Renderer *render = new Renderer(TextureLibrary::instance.getTexture(mesh->getTextureIndex()));
        render->setNeedsLoad();
        render->setCenter(center);
        RenderData *render_data = render->getData();
        
        BlockPos blockpos = chunk->decodeIndex(i);
        render->position = glm::dvec3(blockpos.X+0.5, blockpos.Y+0.5, blockpos.Z+0.5);
        
        int rotation = chunk->getRotation(i);
        int show_faces = facing::rotateFaces(block_show_faces[i], rotation);        
        if (rotation) {
            rot_matrix = Mesh::getRotationMatrix(rotation);
            mesh->getTriangleVertices(show_faces, render_data->vertices, rot_matrix, blockpos, center);
            mesh->getTriangleNormals(show_faces, render_data->normals, rot_matrix);
        } else {
            mesh->getTriangleVertices(show_faces, render_data->vertices, blockpos, center);
            mesh->getTriangleNormals(show_faces, render_data->normals);
        }

        mesh->getTriangleTexCoords(show_faces, render_data->texcoords);
        render_data->total_vertices += mesh->numTriangleVertices(show_faces);
        
        new_trans->push_back(render);
    }
    
    // std::cout << "Trans made renders: " << new_trans->size() << std::endl;
    
    // Get a copy of the old alt
    std::vector<Renderer *> *old_trans = trans_alt;
    COMPILER_BARRIER();
    // Shift old trans into alt and new trans into trans
    trans_alt = trans;
    trans = new_trans;
    COMPILER_BARRIER();
    // Delete the old alt
    if (old_trans) {
        // std::cout << "Deleting trans renderer size=" << old_trans->size() << std::endl;
        RenderManager::instance.queueDeleteRenderer(*old_trans);
        delete old_trans;
    }
}

void ChunkView::copyTransRenders(std::vector<Renderer *>& all_trans)
{
    std::vector<Renderer *> *old_trans = trans;
    COMPILER_BARRIER();
    if (!old_trans) return;
    all_trans.insert(std::end(all_trans), std::begin(*old_trans), std::end(*old_trans));
}


bool ChunkView::insideFrustum(const glm::mat4& view, const BlockPos& center)
{
    if (!chunk) return false;
    BlockPos corners[8];
    chunk->getCorners(corners, center);
    glm::mat4 xform = projection * view;
    
    int reduction = -1;
    for (int i=0; i<8; i++) {
        int outside = WorldView::outsideFrustum(xform, corners[i]);
        reduction &= outside;
    }
    
    return reduction == 0;
}

void ChunkView::computeUpdates(const BlockPos& center, const glm::mat4& projection, const glm::mat4& view)
{
    setProjection(projection);
    // BlockPos center = geom::computeCenter(camera_pos);
    // glm::mat4 view = camera->getViewMatrix(center.X, center.Y, center.Z);
    
    if (!insideFrustum(view, center)) {
        // std::cout << "Skipping chunk\n";
        // render_is_valid = false;
        return;
    }
    
    if (chunk_visual_modified) {
        chunk_visual_modified = false;

        updateAllBlockFaces();
        computeAllRenders(center);
        transIterateBlocks(center);
    }

    COMPILER_BARRIER();
    render_is_valid = true;
}

void ChunkView::draw(Shader *shader, CameraModel *camera)
{
    bool did_frustum_check = false;
    
    if (!render_is_valid) return;
    COMPILER_BARRIER();
    
    size_t num_tex = render.size();
    // std::cout << "Drawing chunk " << chunk->chunk_pos.toString() << " num_tex:" << num_tex << std::endl;
    for (int i=0; i<num_tex; i++) {
        COMPILER_BARRIER();
        if (!render_is_valid) return;
        COMPILER_BARRIER();

        Renderer *mr = render[i];
        if (!mr) continue;
        const BlockPos& center(mr->getCenter());
        glm::mat4 view = camera->getViewMatrix(center.X, center.Y, center.Z);
        
        if (!did_frustum_check) {
            if (!insideFrustum(view, center)) return;
            did_frustum_check = true;
        }
        
        shader->setMat4("view", view);
        mr->load_buffers();
        mr->draw(shader);
    }
}
