#include "gl_includes.hpp"
#include "uielements.hpp"
#include "shader.hpp"
#include "block.hpp"
#include "chunk.hpp"
#include "world.hpp"
#include "compat.hpp"
#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "time.hpp"
#include "blocklibrary.hpp"
#include <algorithm>

UIElements UIElements::instance;


static const unsigned int face_indices[] = {
    0, 1, 2, 0, 2, 3
};

float highlight_faces[] = {
// face:bottom:
    0, 0, 1,
    0, 0, 0,
    1, 0, 0,
    1, 0, 1,
// face:top:
    0, 1, 0,
    0, 1, 1,
    1, 1, 1,
    1, 1, 0,
// face:north:
    1, 1, 0,
    1, 0, 0,
    0, 0, 0,
    0, 1, 0, 
// face:south:
    0, 1, 1,
    0, 0, 1,
    1, 0, 1,
    1, 1, 1,
// face:west:
    0, 1, 0,
    0, 0, 0,
    0, 0, 1,
    0, 1, 1,
// face:east:
    1, 1, 1,
    1, 0, 1,
    1, 0, 0,
    1, 1, 0,
};

void UIElements::getHighlightVertices(std::vector<float>& vertices_out, const BlockPos& pos, const BlockPos& center, int face)
{
    int offsetX = pos.X - center.X;
    int offsetY = pos.Y - center.Y;
    int offsetZ = pos.Z - center.Z;
    
    for (int i=0; i<6; i++) {
        int v = face_indices[i];
        float *p = highlight_faces + face*12 + v*3;
        vertices_out.push_back((p[0]-0.5f)*1.001f+0.5f + offsetX);
        vertices_out.push_back((p[1]-0.5f)*1.001f+0.5f + offsetY);
        vertices_out.push_back((p[2]-0.5f)*1.001f+0.5f + offsetZ);
    }
}


void UIElements::drawBlockHighlight(CameraModel *camera, const BlockPos& pos, int show_faces)
{
    glm::dvec3 camera_pos(camera->getPos());
    BlockPos center(0,0,0);
    center.X = double2int(camera_pos.x);
    center.Y = double2int(camera_pos.y);
    center.Z = double2int(camera_pos.z);
        
    std::vector<float> vertices;
    int num_vertices = POPCOUNT(show_faces) * 6;
    for (int f=0; f<facing::NUM_FACES; f++) {
        if ((show_faces>>f)&1) {
            getHighlightVertices(vertices, pos, center, f);
        }
    }
    
    // std::cout << "Numvertices:" << num_vertices << " list:" << vertices.size() << std::endl;
    // std::cout << "center: " << center.toString() << std::endl;
    // for (int i=0; i<vertices.size(); i+=3) {
    //     std::cout << vertices[i] << ' ';
    //     std::cout << vertices[i+1] << ' ';
    //     std::cout << vertices[i+2] << '\n';
    // }
    
    if (!outline_VAO) glGenVertexArrays(1, &outline_VAO);
    glBindVertexArray(outline_VAO);
    outlineShader.setMat4("view", camera->getViewMatrix(center.X, center.Y, center.Z));
    if (!outline_VBO) glGenBuffers(1, &outline_VBO);
    glBindBuffer(GL_ARRAY_BUFFER, outline_VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0); 
    glEnableVertexAttribArray(0);
    
    // glEnable(GL_LINE_SMOOTH);
    // glLineWidth(5);
    glDrawArrays(GL_TRIANGLES, 0, num_vertices);
    glBindVertexArray(0);
}

void UIElements::drawCrosshair(int window_width, int window_height, float factor)
{
    std::vector<float> points;
    
    float length = factor * window_height;
    
    points.push_back(-length/window_width);
    points.push_back(0);
    points.push_back(0);
    points.push_back(length/window_width);
    points.push_back(0);
    points.push_back(0);

    points.push_back(0);
    points.push_back(-length/window_height);
    points.push_back(0);
    points.push_back(0);
    points.push_back(length/window_height);
    points.push_back(0);
    
    const int num_vertices = 4;

    if (!cross_VAO) glGenVertexArrays(1, &cross_VAO);
    glBindVertexArray(cross_VAO);
    crossShader.use();
    if (!cross_VBO) glGenBuffers(1, &cross_VBO);
    glBindBuffer(GL_ARRAY_BUFFER, cross_VBO);
    glBufferData(GL_ARRAY_BUFFER, points.size() * sizeof(float), points.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0); 
    glEnableVertexAttribArray(0);
    glDrawArrays(GL_LINES, 0, num_vertices);
    glBindVertexArray(0);    
}

int num_cells = 10;

std::string UIElements::drawBlockMenu(int window_width, int window_height, double mousex, double mousey)
{
    std::string selected_block;
    
    glm::mat4 view_base, proj, view;
    int selected_cell = -1;
    
    if (window_width >= window_height) {
        float h = 1;
        float w = (float)window_width / (float)window_height;
        float gap = w/2 - h/2;
        proj = glm::ortho(-gap*num_cells, (1+gap)*num_cells, 0.0f, (float)num_cells, -10.0f, 10.0f);
        // float cell_size = (float)window_height / (float)num_cells;
        
        mousex /= window_height;
        mousex -= gap;
        mousey /= window_height;
        int cell_x = (int)floor(mousex * num_cells);
        int cell_y = (int)floor(mousey * num_cells);
        if (cell_x >= 0 && cell_x < num_cells && cell_y >= 0 && cell_y < num_cells) {
            selected_cell = cell_x + cell_y * num_cells;
        } else {
            selected_cell = -1;
        }
    } else {
        float h = (float)window_height / (float)window_width;
        float w = 1;
        float gap = h/2 - w/2;        
        proj = glm::ortho(0.0f, (float)num_cells, -gap*num_cells, (1+gap)*num_cells, -10.0f, 10.0f);
        // float cell_size = (float)window_width / (float)num_cells;

        mousex /= window_width;
        mousey /= window_width;
        mousey -= gap;
        int cell_x = (int)floor(mousex * num_cells);
        int cell_y = (int)floor(mousey * num_cells);
        if (cell_x >= 0 && cell_x < num_cells && cell_y >= 0 && cell_y < num_cells) {
            selected_cell = cell_x + cell_y * num_cells;
        } else {
            selected_cell = -1;
        }
    }
    blocklistShader.setMat4("projection", proj);


    double now = ref::currentTime();
    view_base = glm::mat4(1.0f);
    view_base = glm::translate(glm::mat4(1.0f), glm::vec3(-0.5f, -0.5f, -0.5f)) * view_base;
    view_base = glm::scale(glm::mat4(1.0f), glm::vec3(0.4f, 0.4f, 0.4f)) * view_base;
    view_base = glm::rotate(glm::mat4(1.0f), glm::radians(20.0f), glm::vec3(1.0f, 0.0f, 0.0f)) * view_base;
    view_base = glm::rotate(glm::mat4(1.0f), glm::radians(20.0f), glm::vec3(0.0f, 0.0f, 1.0f)) * view_base;
    view_base = glm::rotate(glm::mat4(1.0f), (float)now, glm::vec3(0.0f, 1.0f, 0.0f)) * view_base;
    // view_base = glm::translate(glm::mat4(1.0f), glm::vec3(0.5f, 0.5f, 0.5f)) * view_base;
    
    std::vector<std::string> bt_list;
    BlockLibrary::instance.getBlockNames(bt_list);
    std::sort(bt_list.begin(), bt_list.end());
    
    glEnable(GL_BLEND);
    
    for (int cell=0; cell<bt_list.size(); cell++) {
        int cell_x = cell % num_cells;
        int cell_y = cell / num_cells;
    
        bool selected = cell == selected_cell;
        
        if (selected) {
            view = glm::scale(glm::mat4(1.0f), glm::vec3(1.5f, 1.5f, 1.5f)) * view_base;
        } else {
            view = view_base;
        }
        view = glm::translate(glm::mat4(1.0f), glm::vec3(cell_x + 0.5f, (num_cells - 1 - cell_y) + 0.5f, 0.5f)) * view;
        blocklistShader.setMat4("view", view);
    
        if (selected) selected_block = bt_list[cell];
        BlockType *bt = BlockLibrary::instance.getBlockType(bt_list[cell]);
        MeshPtr mesh = bt->getMesh();
    
        blocklist_render->setTexture(TextureLibrary::instance.getTexture(mesh->getTextureIndex()));
        blocklist_render->setCenter(BlockPos(0,0,0));
        blocklist_render->setNeedsLoad();
    
        RenderData *render_data = blocklist_render->getData();
        render_data->clear();
        mesh->getTriangleVertices(facing::ALL_FACES, render_data->vertices, BlockPos(0,0,0), BlockPos(0,0,0));
        mesh->getTriangleTexCoords(facing::ALL_FACES, render_data->texcoords);
        mesh->getTriangleNormals(facing::ALL_FACES, render_data->normals);
        render_data->total_vertices += mesh->numTriangleVertices(facing::ALL_FACES);
    
        blocklist_render->load_buffers();
        blocklist_render->draw(&blocklistShader);    
    }
    
    return selected_block;
}