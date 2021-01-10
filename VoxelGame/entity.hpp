#ifndef INCLUDED_ENTITY_HPP
#define INCLUDED_ENTITY_HPP

// #include "position.hpp"
#include "geometry.hpp"
#include "texture.hpp"
#include "mesh.hpp"
#include "render.hpp"
#include "cameramodel.hpp"

class Entity;
typedef std::shared_ptr<Entity> EntityPtr;

class Entity {
    geom::EntityBox ebox;
    glm::dvec3 velocity, accel;
    bool gravity, on_ground;
    
    glm::dvec3 target_pos, current_pos;
    double target_time, current_time;
    // double last_position_update;
    // std::mutex pos_mutex;
    
    double yaw; // XXX rotate the mesh
    bool visible;
    MeshPtr mesh;
    Renderer *render, *render_alt;
    bool visual_modified;
    
    // XXX view
    glm::mat4 projection;
    void setProjection(const glm::mat4& pr) {
        projection = pr;
    }
    
    void computeRender(RenderData *render_data, const BlockPos& center, const glm::dvec3& pos);
        
protected:
    Entity();
    
public:
    
    ~Entity();
    
    void move(const glm::dvec3& motion);
    
    glm::dvec3 getCameraPos() const {
        glm::dvec3 pos = ebox.position;
        pos.y += ebox.height*0.9;
        return pos;
    }
    
    void setCameraPos(const glm::dvec3& pos) {
        ebox.position = pos;
        ebox.position -= ebox.height;
        visual_modified = true; // XXX may be optional
    }
    
    void setPosition(const glm::dvec3& pos) {
        ebox.position = pos;
        visual_modified = true;
    }
        
    void setMesh(MeshPtr m) {
        mesh = m;
        visual_modified = true;
    }
    
    void setVisible(bool v) {
        visible = v;
        visual_modified = true;
    }
    
    void setGravity(bool g) {
        gravity = g;
    }
    
    bool hasGravity() {
        return gravity;
    }
    
    bool isOnGround() {
        return on_ground;
    }
    
    glm::dvec3& getVelocity() { return velocity; }
    void setVelocity(const glm::dvec3& v) { velocity = v; }
    void addVelocity(const glm::dvec3& dv) {
        velocity += dv;
    }
    
    void gameTick(double elapsed_time);
    void setHorizontalVelocity(double vx, double vz);
    void setVerticalVelocity(double vy);
    
    // XXX view
    void computeUpdates(const BlockPos& center, const glm::mat4& projection, const glm::mat4& view);
    bool insideFrustum(const glm::mat4& view, const BlockPos& center);
    void draw(Shader *shader, CameraModel *camera);
    
    static EntityPtr makeEntity() { return EntityPtr(new Entity); }
};



#endif