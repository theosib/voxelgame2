
#include "world.hpp"
#include "entity.hpp"
#include <glm/gtx/string_cast.hpp>
#include <glm/gtc/epsilon.hpp>
#include "time.hpp"

Entity::Entity() {
    ebox.position = glm::dvec3(10, 10, 10);
    ebox.height = 1.8;
    ebox.width = 0.6;
    velocity = glm::dvec3(0,0,0);
    accel = glm::dvec3(0,0,0);
    gravity = true;
    on_ground = false;
    render = 0;
    render_alt = 0;
    // last_position_update = ref::currentTime();
}

Entity::~Entity()
{
    if (render) RenderManager::instance.queueDeleteRenderer(render);
    if (render_alt) RenderManager::instance.queueDeleteRenderer(render_alt);
}

static constexpr double max_y_vel = 50;
// static constexpr double max_y_vel = 1;


void Entity::move(const glm::dvec3& motion)
{
    if (motion.x==0 && motion.y==0 && motion.z==0) return;
    
    // std::cout << "Motion: " << glm::to_string(motion) << std::endl;
    
    geom::Box box_here(ebox);
    geom::BoxArray collision_here;
    World::instance.allIntersectingCollisions(collision_here.arr, box_here);
    collision_here.sort();
    
    if (motion.y != 0) {
        geom::Box box_there(box_here.offset(0, motion.y, 0));
        geom::BoxArray collision_there;
        World::instance.allIntersectingCollisions(collision_there.arr, box_there);
        collision_there.sort();
        
        geom::BoxArray target(collision_there.difference(collision_here));
        
        double d = target.minDistance(box_here);
        if (d >= fabs(motion.y)) {
            ebox.position.y += motion.y;
            on_ground = false;
        } else {
            if (motion.y < 0) {
                // d -= 0.001;
                ebox.position.y -= d;
                on_ground = true;
            } else {
                d -= 0.001;
                ebox.position.y += d;
                on_ground = false;
            }
            velocity.y = 0;
            accel.y = 0;
        }
        
        if (ebox.position.y < 0) {
            ebox.position.y = 0;
            velocity.y = 0;
            accel.y = 0;
            on_ground = true;
        }
    }
    
    if (motion.x==0 && motion.z==0) {
        // std::cout << "Move to: " << glm::to_string(ebox.position) << std::endl;
        visual_modified = true;
        return;
    }
    
    box_here = geom::Box(ebox);
    geom::EntityBox tall_ebox = ebox;
    tall_ebox.height += 0.625;
    geom::Box tall_box_here(tall_ebox);
    geom::BoxArray tall_collision_here;
    World::instance.allIntersectingCollisions(tall_collision_here.arr, tall_box_here);
    tall_collision_here.sort();
    
    geom::BoxArray target_x, target_z, target_y;
    if (motion.x != 0) {
        geom::BoxArray collision_there_x;
        geom::Box tall_box_there(tall_box_here.offset(motion.x, 0, 0));
        World::instance.allIntersectingCollisions(collision_there_x.arr, tall_box_there);
        collision_there_x.sort();
        target_x = collision_there_x.difference(tall_collision_here);
    }
    if (motion.z != 0) {
        geom::BoxArray collision_there_z;
        geom::Box tall_box_there(tall_box_here.offset(0, 0, motion.z));
        World::instance.allIntersectingCollisions(collision_there_z.arr, tall_box_there);
        collision_there_z.sort();
        target_z = collision_there_z.difference(tall_collision_here);
    }
    {
        geom::BoxArray collision_there_y;
        geom::Box box_there(box_here.offset(0, 0.625, 0));
        World::instance.allIntersectingCollisions(collision_there_y.arr, box_there);
        collision_there_y.sort();
        target_y = collision_there_y.difference(collision_here);
    }
    
    double expected_mx = fabs(motion.x);
    double expected_mz = fabs(motion.z);
    
    bool best_coll_x = true, best_coll_z = true;
    double best_step = 0;
    double best_dd = 0, best_dx = 0, best_dz = 0;
    for (double step=0; step<=0.625; step+=0.0625) {
        geom::Box up = box_here.offset(0, step, 0);
        double d_y = target_y.minDistance(up) - 0.001;
        if (d_y <= 0 && step>0) break;
        
        double d_x = 0, d_z = 0;        
        if (motion.x!=0) d_x = target_x.minDistance(up) - 0.001;
        if (motion.z!=0) d_z = target_z.minDistance(up) - 0.001;
        
        // std::cout << "step=" << step << " dx=" << d_x << " dz=" << d_z << std::endl;
        
        if (d_x < 0) d_x = 0;
        if (d_z < 0) d_z = 0;
        
        if (d_x >= expected_mx && d_z >= expected_mz) {
            best_step = step;
            best_dx = expected_mx;
            best_dz = expected_mz;
            best_coll_z = false;
            best_coll_x = false;
            break;
        }
        
        bool coll_x = false;
        if (d_x >= expected_mx) {
            d_x = expected_mx;
        } else {
            coll_x = true;
        }
        
        bool coll_z = false;
        if (d_z >= expected_mz) {
            d_z = expected_mz;
        } else {
            coll_z = true;
        }
        
        double dd = d_x*d_x + d_z*d_z;
        if (dd > best_dd) {
            best_dd = dd;
            best_dx = d_x;
            best_dz = d_z;
            best_step = step;
            best_coll_z = coll_z;
            best_coll_x = coll_x;
        }
        
        if (!on_ground || !gravity) break;
    }
    
    ebox.position.y += best_step;
    if (motion.x<0) {
        ebox.position.x -= best_dx;
    } else {
        ebox.position.x += best_dx;
    }
    if (motion.z<0) {
        ebox.position.z -= best_dz;
    } else {
        ebox.position.z += best_dz;
    }
    if (best_coll_x) {
        velocity.x = 0;
        accel.x = 0;
    }
    if (best_coll_z) {
        velocity.z = 0;
        accel.z = 0;
    }
        
    // XXX may be optional
    // std::cout << "Move to: " << glm::to_string(ebox.position) << std::endl;
    visual_modified = true;
}



void Entity::gameTick(double elapsed_time)
{
    // std::unique_lock<std::mutex> lock(pos_mutex);
    // std::cout << "tick " << elapsed_time << std::endl;
    
    static constexpr double third = 1.0 / 3.0;
    double step = elapsed_time * third; // 50 ms game tick
    if (gravity) {
        //accel = glm::dvec3(0, -9, 0);
        accel.y = -20;
    }
    
    // std::cout << "Velocity: " << glm::to_string(velocity) << std::endl;
    
    glm::dvec3 pos = ebox.position;
    for (int i=0; i<3; i++) {
        // Update position from prior velocity
        pos += velocity * step;
        
        // Apply friction
#if 1
        if (on_ground && gravity) {
            glm::dvec3 hvel = velocity;
            hvel.y = 0;
            if (glm::any(glm::greaterThan(glm::abs(hvel), glm::dvec3(0.1)))) {
                hvel = glm::normalize(hvel);
            }
            hvel *= 100;
            glm::dvec3 friction(-hvel.x, 0.0, -hvel.z);
            friction *= step;
            if (fabs(friction.x) > fabs(velocity.x)) friction.x = -velocity.x;
            if (fabs(friction.z) > fabs(velocity.z)) friction.z = -velocity.z;
            velocity += friction;
        }
#endif
        
        // Update velocity from accel
        velocity += step * accel;
        if (velocity.y < -max_y_vel) velocity.y = -max_y_vel;
        if (velocity.y > max_y_vel) velocity.y = max_y_vel;
    }
    move(pos - ebox.position);
    // last_position_update = ref::currentTime() + 0.05;
    
}

void Entity::setHorizontalVelocity(double vx, double vz)
{
    velocity.x = vx;
    velocity.z = vz;
    // XXX apply limit
}

void Entity::setVerticalVelocity(double vy)
{
    if (vy < -max_y_vel) vy = -max_y_vel;
    if (vy > max_y_vel) vy = max_y_vel;
    velocity.y = vy;
}

void Entity::computeUpdates(const BlockPos& center, const glm::mat4& projection, const glm::mat4& view)
{
    // std::unique_lock<std::mutex> lock(pos_mutex);

    if (!visible) return;    
    setProjection(projection); // Necessary here?
    if (!visual_modified) return;
    
    if (!insideFrustum(view, center)) return;
    visual_modified = false;
    
    glm::dvec3 pos = ebox.position;
    
    Renderer* mr1 = render_alt;
    if (!mr1) {
        mr1 = new Renderer(TextureLibrary::instance.getTexture(mesh->getTextureIndex()));
        current_pos = pos;
        current_time = ref::currentTime();
    }
    
    target_pos = pos;
    target_time = ref::currentTime() + 0.05;
    
    // Save center in render for proper alignment when rendering
    mr1->setCenter(center);
    computeRender(mr1->getData(), center, pos);
    
    mr1->setNeedsLoad(); // XXX Maybe optional
    
    Renderer* mr2 = render;
    if (!mr2) mr2 = new Renderer(TextureLibrary::instance.getTexture(mesh->getTextureIndex()));
    
    // Swap updated render into place
    render_alt = mr2;
    render = mr1;
}

// double last_compute_render;
void Entity::computeRender(RenderData *render_data, const BlockPos& center, const glm::dvec3& pos)
{    
    render_data->clear();    
    mesh->getTriangleVertices(facing::ALL_FACES, render_data->vertices, pos, center);
    mesh->getTriangleTexCoords(facing::ALL_FACES, render_data->texcoords);
    mesh->getTriangleNormals(facing::ALL_FACES, render_data->normals);
    render_data->total_vertices += mesh->numTriangleVertices(facing::ALL_FACES);
}

bool Entity::insideFrustum(const glm::mat4& view, const BlockPos& center)
{
    // XXX
    return true;
}

double last_y;
void Entity::draw(Shader *shader, CameraModel *camera)
{
    // std::unique_lock<std::mutex> lock(pos_mutex);
    
    static constexpr double frame = 1.0 / 60.0;
    if (!visible) return;

    Renderer *mr = render;
    if (!mr) return;
    const BlockPos& center(mr->getCenter());
    glm::mat4 view = camera->getViewMatrix(center.X, center.Y, center.Z);
    if (!insideFrustum(view, center)) return;
    
    
/*    glm::dvec3 pos = mr->pos;
    glm::dvec3 vel = mr->vel;
    glm::dvec3 acc = mr->acc;
    double now = ref::currentTime();
    double elapsed = now - mr->last_position_update;
    double elapsed2 = elapsed;
    // std::cout << "elapsed=" << elapsed << " frame=" << frame << " self=" << (now - last_compute_render) << std::endl;
    // last_compute_render = now;
    while (elapsed > 0) {
        double step = elapsed;
        if (step > frame) step = frame;
        
        pos += vel * step;
        vel += step * accel;
        if (vel.y < -max_y_vel) vel.y = -max_y_vel;
        if (vel.y > max_y_vel) vel.y = max_y_vel;
        
        // std::cout << "intermediate\n";
        elapsed -= step;
    }*/
    
    glm::vec3 offset(0,0,0);
    
    double now = ref::currentTime();
    double elapsed = now - current_time;
    double togo = target_time - now;
    if (togo > 0) {
        glm::dvec3 pos = (target_pos * elapsed + current_pos * togo) / (target_time - current_time);
        // std::cout << "pos=" << glm::to_string(pos) << " target=" << glm::to_string(target_pos) << " current=" << glm::to_string(current_pos) << std::endl;
        offset = pos - target_pos;
        current_pos = pos;
    } else {
        current_pos = target_pos;
    }
    current_time = now;
        
    /*glm::dvec3 pos1 = mr->pos;
    glm::dvec3 pos2 = position.position;
    double now = ref::currentTime();
    double elapsed = now - mr->last_position_update;
    double togo = last_position_update - now;
    glm::dvec3 pos = (pos2 * elapsed + pos1 * togo) / (elapsed + togo);
    mr->pos = pos;
    mr->last_position_update = now;*/
    
    // std::cout << "elapsed=" << elapsed2 << " offset=" << (pos.y - mr->pos.y) << std::endl;
    // glm::vec3 offset = glm::vec3(pos - position.position);
    // std::cout << glm::to_string(offset) << " " << glm::to_string(pos) << " " << glm::to_string(mr->pos) << std::endl;
    glm::mat4 model = glm::translate(glm::mat4(1.0f), offset);
    shader->setMat4("view", view);
    shader->setMat4("model", model);
    mr->load_buffers();
    mr->draw(shader);    
}
