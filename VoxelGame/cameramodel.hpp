#ifndef INCLUDED_CAMERA_MODEL
#define INCLUDED_CAMERA_MODEL

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <mutex>
#include "entity.hpp"

class CameraModel {    
    std::mutex camera_mutex;
    Entity entity;
    double movement_speed;
    
public:
    glm::dvec3 world_up;
    // glm::dvec3 position;
    glm::dvec3 cam_front, cam_up, cam_right;
    glm::dvec3 move_front, move_up, move_right;
    double pitch, yaw;
    
    CameraModel(const glm::dvec3& pos_in, double yaw_in, double pitch_in);
    CameraModel(double posX, double posY, double posZ, double yaw_in, double pitch_in);
    
    void move(const glm::dvec3& motion);
    void moveHorizontally(const glm::dvec3& vec);
    void moveForward(double deltaTime);
    void moveBackward(double deltaTime);
    void moveLeft(double deltaTime);
    void moveRight(double deltaTime);
    void moveUp(double deltaTime);
    void moveDown(double deltaTime);
    void gameTick(double deltaTime);
    void toggleFly() {
        entity.setGravity(!entity.hasGravity());
    }
    
    void rotate(double pitch_change, double yaw_change);    
    
    glm::mat4 getViewMatrix(double cx, double cy, double cz);
    
    glm::dvec3 getPos() const {
        // return position;
        return entity.getCameraPos();
    }
    
    const glm::dvec3& getForward() const {
        return cam_front;
    }
    
private:
    void update();
};

#endif
