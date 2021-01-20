#include "gl_includes.hpp"
#include <stdio.h>
#include <iostream>
#include <iomanip>
#include <math.h>
#include "facing.hpp"
#include "gamewindow.hpp"
#include "texture.hpp"
#include "world.hpp"
#include "worldview.hpp"
#include "compat.hpp"
#include "uielements.hpp"
// #include "longconcurrentmap.hpp"
#include <stdlib.h>
#include <glm/gtx/string_cast.hpp>
#include <glm/gtc/matrix_access.hpp>
#include <glm/gtc/type_ptr.hpp>

GameWindow window;
// CameraController camera(0, 0.5, 50, -90, 0);
CameraController camera(0.5, 5, 0.5, -90, 0);


int which = 0;
void makeSphere()
{
    std::string blocks[4];
    blocks[0] = "cobblestone";
    blocks[1] = "wood";
    blocks[2] = "steel";
    blocks[3] = "dirt";
    
    for (int x=-40; x<=40; x++) {
        for (int y=-40; y<=40; y++) {
            for (int z=-40; z<=40; z++) {
                double r = x*x + y*y + z*z;
                if (r <= 900 && r >= 100) {
                    BlockPos pos(x, y, z);
                    
                    // int which = random() % 4;
                    const std::string& block(blocks[which]);
                    
                    World::instance.queueBlock(pos, block);
                }
            }
        }
    }
    World::instance.flushBlockQueue();
    which = (which + 1) & 3;
}

// int insideFrustum(const glm::mat4& transform, const glm::vec3& point_in)
// {
//     int result = 0;
//
//     glm::vec4 point(point_in, 1.0f);
//     glm::vec4 row1(glm::row(transform, 0));
//     glm::vec4 row2(glm::row(transform, 1));
//     glm::vec4 row3(glm::row(transform, 2));
//     glm::vec4 row4(glm::row(transform, 3));
//
//     if (0 <= glm::dot(point, row4 + row1)) result |= facing::WEST_MASK;
//     if (0 <= glm::dot(point, row4 - row1)) result |= facing::EAST_MASK;
//     if (0 <= glm::dot(point, row4 + row2)) result |= facing::DOWN_MASK;
//     if (0 <= glm::dot(point, row4 - row2)) result |= facing::UP_MASK;
//     if (0 <= glm::dot(point, row4 + row3)) result |= facing::SOUTH_MASK;
//     if (0 <= glm::dot(point, row4 - row3)) result |= facing::NORTH_MASK;
//
//     return result;
// }


// bool loop_live = true;
// void sphereSwitch()
// {
//     while (loop_live) {
//         for (int i=0; i<5; i++) {
//             usleep(1000000);
//             if (!loop_live) return;
//         }
//         makeSphere();
//     }
// }

// void test_hash()
// {
//     LongConcurrentMap<int> map;
//
//     map.put(0xaaaaa, 20);
//     int out = map.get(0xaaaaa);
//     std::cout << out << std::endl;
// }


void test()
{
    glm::mat4 rot(1.0f);
    rot = glm::translate(rot, glm::vec3(-0.5f, -0.5f, -0.5f));
    rot = glm::rotate(rot, glm::radians(270.0f), glm::vec3(1.0f, 0.0f, 0.0f));
    rot = glm::translate(rot, glm::vec3(0.5f, 0.5f, 0.5f));
    std::cout << glm::to_string(rot) << std::endl;
    exit(0);
}

// void chunkSaveLoop()
// {
//     while (loop_live) {
//         usleep(10000);
//         Chunk *chunk = World::instance.nextSaveChunk();
//         if (chunk) {
//             World::instance.saveChunk(chunk);
//         }
//     }
// }


void register_static_blocks();
void init_dirt_block();

EntityPtr my_entity = 0;
void make_entity()
{
    MeshPtr mesh = Mesh::makeMesh();
    mesh->loadMesh("stone");
    my_entity = Entity::makeEntity();
    my_entity->setMesh(mesh);
    my_entity->setPosition(glm::dvec3(0, 30, 0));
    my_entity->setVisible(true);
    World::instance.addEntity(my_entity);
}

void rotation_test();

#if defined(_DEBUG) || defined(__APPLE__)
int main()
#else
int WinMain()
#endif
{
    double hide_preview_after = 0;
    std::string selected_block;
    
    std::cout << std::fixed << std::setprecision(20);
    
    // rotation_test();
    // exit(0);
    
    register_static_blocks();
    init_dirt_block();
    // World::instance.setBlock(BlockPos(1,0,0), "wood");
    // World::instance.setBlock(BlockPos(0,0,0), "wood");
    
    // for (int i=0; i<24; i++) {
    //     World::instance.setBlock(BlockPos(i*4, 4, 0), "numbercube", 0);
    //     World::instance.setBlock(BlockPos(i*4, 4, 4), "numbercube", i);
    // }

    // makeSphere();
    
    // std::thread switchnig(sphereSwitch);
    // std::thread saveThread(chunkSaveLoop);
    

    window.create();
    window.setCamera(&camera);
    window.setFOV(glm::radians(45.0f));

    std::cout << "Launching thread\n";
    World::instance.startLoadSaveThread();
    World::instance.startTickThread();
    World::instance.startBlockUpdateThread();
    RenderManager::instance.launch_threads();
    
    // make_entity();
    
    CameraModel *model = camera.getModel();

    int counter = 0;
    double last_frame_time = glfwGetTime();
    while (!window.shouldClose()) {
        
        // my_entity->move(glm::dvec3(0.01, 0, 0));
        
        double now = glfwGetTime();
        double deltaTime = now - last_frame_time;
        last_frame_time = now;
        camera.gameTick(deltaTime);
        
        
        glm::mat4 projection = window.getProjectionMatrix();

        glEnable(GL_DEPTH_TEST);
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        WorldView::instance.setProjection(projection);
        WorldView::instance.draw(model);
        
        // glDisable(GL_CULL_FACE);
        
        BlockPos target;
        double dist;
        int enter_face = -1;
        if (!window.isShowingMenu()) {
            World::instance.findNearestBlock(model->getPos(), model->getForward(), 10, target, dist, enter_face);
            if (enter_face>=0) {
                // std::cout << "Hit " << target.toString() << " d=" << dist << " face=" << enter_face << std::endl;
                UIElements::instance.setHighlightProjection(projection);
                glEnable(GL_BLEND);
                glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                UIElements::instance.drawBlockHighlight(model, target, facing::bitmask(enter_face));
                
                const std::string& block(World::instance.getBlockToPlace());
                int rot = World::instance.getBlockRotation();
                if (block.size() > 0 && now <= hide_preview_after) {
                    glEnable(GL_DEPTH_TEST);
                    glEnable(GL_CULL_FACE);
                    glCullFace(GL_BACK);
                    WorldView::instance.drawOneBlock(model, target.neighbor(enter_face), block, rot);
                }
                glDisable(GL_BLEND);
            }
        }
        
        if (!window.isShowingMenu()) {
            glDisable(GL_DEPTH_TEST);
            UIElements::instance.drawCrosshair(window.getWidth(), window.getHeight(), 0.03f);
        }
        
        if (window.isShowingMenu()) {
            glClear(GL_DEPTH_BUFFER_BIT);
            glEnable(GL_DEPTH_TEST);
            glEnable(GL_CULL_FACE);
            glCullFace(GL_BACK);
            double mx, my;
            window.getCursorPos(&mx, &my);
            selected_block = UIElements::instance.drawBlockMenu(window.getWidth(), window.getHeight(), mx, my);
        } else {
            selected_block = "";
        }
        
        // Get input just before processing block updates
        window.poll_events();
        window.process_input();
        
        // Do any block changes and fire off mesh compute so that happens in parallel to 
        // the vsync wait and then appears ASAP on the frame after this one.
        
        
        if (window.isShowingMenu()) {
            if (window.checkLeftButton()) {
                World::instance.setBlockToPlace(selected_block);
                World::instance.setBlockRotation(0);
                window.showMenu(false);
                hide_preview_after = now + 2;
            }
        } else {
            if (enter_face>=0) {
                if (window.checkLeftButton()) {
                    World::instance.hitAction(target, enter_face);
                    hide_preview_after = now + 2;
                }
                if (window.checkRightButton()) {
                    World::instance.useAction(target, enter_face);
                    hide_preview_after = now + 2;
                }
                int inc = window.getScrollDir();
                if (inc) {
                    World::instance.incBlockRotation(inc);
                    std::cout << "Rotation: " << World::instance.getBlockRotation() << std::endl;
                    hide_preview_after = now + 2;
                }
            }
        }
        window.clearLeftButton();
        window.clearRightButton();
        window.clearScrollDir();
        
        RenderManager::instance.deleteDeadRendererQueue();
        RenderManager::instance.setCamera(model);
        RenderManager::instance.setProjection(projection);
        RenderManager::instance.signalComputeRenders();
        
        World::instance.setUserPosition(BlockPos(model->getPos()));
        
        // XXX These and all other setting of blocks should be done in a block setting thread!
        // World::instance.doBlockUpdates();

        // window.next_frame();
        window.swap_buffers();
    }
    
    World::instance.saveAll();
    
    RenderManager::instance.stop();
    World::instance.stopLoadSaveThread();
    World::instance.stopBlockUpdateThread();
    World::instance.stopTickThread();
    
    // loop_live = false;
    // saveThread.join();
    // switchnig.join();
}
