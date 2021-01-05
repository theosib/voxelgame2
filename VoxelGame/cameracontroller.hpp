#ifndef INCLUDED_CAMERAMODEL_HPP
#define INCLUDED_CAMERAMODEL_HPP

#include "cameramodel.hpp"

class CameraController {
public:
    CameraModel camera;
    double mouse_sensitivity;
    
    CameraController();
    CameraController(double posX, double posY, double posZ, double yaw_in, double pitch_in);
    
    void moveForward(double deltaTime);
    void moveBackward(double deltaTime);
    void moveLeft(double deltaTime);
    void moveRight(double deltaTime);
    void moveUp(double deltaTime);
    void moveDown(double deltaTime);
    
    void gameTick(double deltaTime);
    void toggleFly() {
        camera.toggleFly();
    }
    
    void mouseMovement(double xoffset, double yoffset);
    
    glm::mat4 getViewMatrix(double cx, double cy, double cz) {
        return camera.getViewMatrix(cx, cy, cz);
    }
    
    CameraModel *getModel() { return &camera; }
    
    glm::dvec3 getPos() {
        return camera.getPos();
    }
};

#endif