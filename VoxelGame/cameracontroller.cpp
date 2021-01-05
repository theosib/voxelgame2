

#include "cameracontroller.hpp"
#include <iostream>

const double SENSITIVITY =  0.1f;


CameraController::CameraController() : camera(0, 0.5, 110, -90, 0)
{
    // movement_speed = SPEED;
    mouse_sensitivity = SENSITIVITY;
}

CameraController::CameraController(double posX, double posY, double posZ, double yaw_in, double pitch_in) :
    camera(posX, posY, posZ, yaw_in, pitch_in)
{
    // movement_speed = SPEED;
    mouse_sensitivity = SENSITIVITY;
}


void CameraController::moveForward(double deltaTime)
{
    camera.moveForward(deltaTime);
}

void CameraController::moveBackward(double deltaTime)
{
    camera.moveBackward(deltaTime);
}

void CameraController::moveLeft(double deltaTime)
{
    camera.moveLeft(deltaTime);
}

void CameraController::moveRight(double deltaTime)
{
    camera.moveRight(deltaTime);
}

void CameraController::moveUp(double deltaTime)
{
    // std::cout << "deltaTime:" << deltaTime << std::endl;
    camera.moveUp(deltaTime);
}

void CameraController::moveDown(double deltaTime)
{
    camera.moveDown(deltaTime);
}

void CameraController::gameTick(double deltaTime)
{
    camera.gameTick(deltaTime);
}



void CameraController::mouseMovement(double xoffset, double yoffset)
{
    xoffset *= mouse_sensitivity;
    yoffset *= mouse_sensitivity;
    
    camera.rotate(yoffset, xoffset);
}
