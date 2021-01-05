#include "gl_includes.hpp"
#include "worldview.hpp"
#include "world.hpp"
#include <iostream>
#include "spinlock.hpp"
#include <glm/gtc/matrix_access.hpp>
#include <glm/gtx/norm.hpp>
#include "geometry.hpp"
#include "blocklibrary.hpp"

WorldView WorldView::instance;

void WorldView::computeChunkRenders(const glm::dvec3& camera_pos, const glm::mat4& projection, CameraModel *camera)
{    
    std::vector<Chunk *> chunks;
    BlockPos center = geom::computeCenter(camera_pos);
    glm::mat4 view_matrix = camera->getViewMatrix(center.X, center.Y, center.Z);
    
    World::instance.listAllChunks(chunks);
    for (auto i=chunks.begin(); i!=chunks.end(); ++i) {
        Chunk *chunk = *i;
        ChunkView *view = chunk->getView();
        view->computeUpdates(center, projection, view_matrix);
    }
}

void WorldView::computeEntityRenders(const glm::dvec3& camera_pos, const glm::mat4& projection, CameraModel *camera)
{    
    std::vector<std::shared_ptr<Entity>> entities;
    BlockPos center = geom::computeCenter(camera_pos);
    glm::mat4 view_matrix = camera->getViewMatrix(center.X, center.Y, center.Z);
    
    World::instance.listAllEntities(entities);    
    for (auto i=entities.begin(); i!=entities.end(); ++i) {
        (*i)->computeUpdates(center, projection, view_matrix);
    }
}

void WorldView::draw(CameraModel *camera)
{
    std::vector<Chunk *> chunks;
    std::vector<std::shared_ptr<Entity>> entities;
    
    World::instance.listAllChunks(chunks);
    
    glDisable(GL_BLEND);
    
    for (auto i=chunks.begin(); i!=chunks.end(); ++i) {
        Chunk *chunk = *i;
        ChunkView *view = chunk->getView();
        if (view) {
            view->draw(&blockShader, camera);
        }
    }

    World::instance.listAllEntities(entities);

    for (auto i=entities.begin(); i!=entities.end(); ++i) {
        (*i)->draw(&entityShader, camera);
    }

    glEnable(GL_BLEND);

    drawTrans(camera, chunks);
}

void WorldView::drawTrans(CameraModel *camera, const std::vector<Chunk *>& chunks)
{
    std::vector<Renderer *> trans;
    
    for (auto i=chunks.begin(); i!=chunks.end(); ++i) {
        Chunk *chunk = *i;
        ChunkView *view = chunk->getView();
        view->copyTransRenders(trans);
    }
    
    glm::dvec3 campos = camera->getPos();
    
    std::sort(trans.begin(), trans.end(),
        [&campos](Renderer *a, Renderer *b) -> bool {
            double da = glm::length2(a->position - campos);
            double db = glm::length2(b->position - campos);
            return db < da; // Sort from far to near
        });
    
    size_t num_tex = trans.size();
    // std::cout << "Drawing trans, num=" << num_tex << std::endl;
    for (int i=0; i<num_tex; i++) {
        Renderer *mr = trans[i];
        if (!mr) continue;
        const BlockPos& center(mr->getCenter());
        glm::mat4 view = camera->getViewMatrix(center.X, center.Y, center.Z);
    
        // if (!did_frustum_check) {
        //     if (!insideFrustum(view, center)) return;
        //     did_frustum_check = true;
        // }
    
        blockShader.setMat4("view", view);
        mr->load_buffers();
        mr->draw(&blockShader);
    }
    
}


void WorldView::drawOneBlock(CameraModel *camera, const BlockPos& pos, const std::string& block_name, int rotation)
{
    BlockPos center = geom::computeCenter(camera->getPos());
    
    BlockType *bt = BlockLibrary::instance.getBlockType(block_name);
    MeshPtr mesh = bt->getMesh();
    
    placeblock_render->setTexture(TextureLibrary::instance.getTexture(mesh->getTextureIndex()));
    placeblock_render->setCenter(center);
    placeblock_render->setNeedsLoad();

    RenderData *render_data = placeblock_render->getData();
    render_data->clear();
    if (rotation) {
        glm::mat4 rot_matrix = Mesh::getRotationMatrix(rotation);
        mesh->getTriangleVertices(facing::ALL_FACES, render_data->vertices, rot_matrix, pos, center);        
        mesh->getTriangleNormals(facing::ALL_FACES, render_data->normals, rot_matrix);
    } else {
        mesh->getTriangleVertices(facing::ALL_FACES, render_data->vertices, pos, center);
        mesh->getTriangleNormals(facing::ALL_FACES, render_data->normals);
    }
    mesh->getTriangleTexCoords(facing::ALL_FACES, render_data->texcoords);
    render_data->total_vertices += mesh->numTriangleVertices(facing::ALL_FACES);
    
    glm::mat4 view = camera->getViewMatrix(center.X, center.Y, center.Z);
    placementShader.setMat4("view", view);

    placeblock_render->load_buffers();
    placeblock_render->draw(&placementShader);    
}

#if 0
std::string error_description(GLenum err) {
    switch(err) {
        case GL_NO_ERROR:
            return "GL_NO_ERROR: No error has been recorded. The value of this symbolic constant is guaranteed to be 0. ";
        case GL_INVALID_ENUM:
            return "GL_INVALID_ENUM: An unacceptable value is specified for an enumerated argument. The offending command is ignored and has no other side effect than to set the error flag.  ";
        case GL_INVALID_VALUE:
            return "GL_INVALID_VALUE: A numeric argument is out of range. The offending command is ignored and has no other side effect than to set the error flag.  ";
        case GL_INVALID_OPERATION:
            return "GL_INVALID_OPERATION: The specified operation is not allowed in the current state. The offending command is ignored and has no other side effect than to set the error flag.  ";
        case GL_INVALID_FRAMEBUFFER_OPERATION:
            return
            "GL_INVALID_FRAMEBUFFER_OPERATION: The framebuffer object is not complete."
            "The offending command is ignored and has no other side effect than to set the error flag.";
        case GL_OUT_OF_MEMORY:
            return "GL_OUT_OF_MEMORY: There is not enough memory left to execute the command. The state of the GL is undefined, except for the state of the error flags, after this error is recorded. . ";
        default:
            return "No Description";
    }
}

void errorCheck() {
    GLenum err;
    while ((err = glGetError()) != GL_NO_ERROR) std::cout << error_description(err) << std::endl;
}
#endif





// http://web.archive.org/web/20120531231005/http://crazyjoke.free.fr/doc/3D/plane%20extraction.pdf
int WorldView::outsideFrustum(const glm::mat4& transform, const glm::dvec3& point_in)
{
    int result = 0;
    
    glm::vec4 point(point_in, 1.0f);
    glm::vec4 row1(glm::row(transform, 0));
    glm::vec4 row2(glm::row(transform, 1));
    glm::vec4 row3(glm::row(transform, 2));
    glm::vec4 row4(glm::row(transform, 3));
    
    if (0 > glm::dot(point, row4 + row1)) result |= facing::WEST_MASK;
    if (0 > glm::dot(point, row4 - row1)) result |= facing::EAST_MASK;
    if (0 > glm::dot(point, row4 + row2)) result |= facing::DOWN_MASK;
    if (0 > glm::dot(point, row4 - row2)) result |= facing::UP_MASK;
    if (0 > glm::dot(point, row4 + row3)) result |= facing::SOUTH_MASK;
    if (0 > glm::dot(point, row4 - row3)) result |= facing::NORTH_MASK;
    
    return result;
}




