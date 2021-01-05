

#include "cameramodel.hpp"
#include <iostream>
#include <glm/gtx/string_cast.hpp>

// static constexpr glm::vec3 world_up(0.0f, 1.0f, 0.0f);

const double SPEED       =  5;

CameraModel::CameraModel(const glm::dvec3& pos_in, double yaw_in, double pitch_in)
{
    // center = glm::dvec3(0,0,0);
    world_up = glm::dvec3(0.0f, 1.0f, 0.0f);
    //position = pos_in;
    entity.setCameraPos(pos_in);
    yaw = yaw_in;
    pitch = pitch_in;
    movement_speed = SPEED;
    update();
}

CameraModel::CameraModel(double posX, double posY, double posZ, double yaw_in, double pitch_in)
{
    // center = glm::dvec3(0,0,0);
    world_up = glm::dvec3(0.0f, 1.0f, 0.0f);
    // position = glm::dvec3(posX, posY, posZ);
    entity.setCameraPos(glm::dvec3(posX, posY, posZ));
    yaw = yaw_in;
    pitch = pitch_in;
    movement_speed = SPEED;
    update();
}

void CameraModel::move(const glm::dvec3& motion)
{
    entity.move(motion);
    // position += motion;
    // update();
}

// inline int sign(double x) {
//     if (x<0) return -1;
//     if (x>0) return 1;
//     return 0;
// }

void CameraModel::moveHorizontally(const glm::dvec3& vec)
{
    double new_x = 0;
    double new_z = 0;
    if (entity.getVelocity().y > 0) {
        double hspeed = movement_speed - fabs(entity.getVelocity().y);
        if (hspeed < 0) hspeed = 0;
        new_z = vec.z * hspeed;
        new_x = vec.x * hspeed;
    } else {
        new_z = vec.z * movement_speed;
        new_x = vec.x * movement_speed;
    }

    double& old_z(entity.getVelocity().z);
    if (fabs(new_z) > fabs(old_z) || (new_z<0 && old_z>0) || (new_z>0 && old_z<0)) old_z = new_z;
    double& old_x(entity.getVelocity().x);
    if (fabs(new_x) > fabs(old_x) || (new_x<0 && old_x>0) || (new_x>0 && old_x<0)) old_x = new_x;
}

void CameraModel::moveForward(double deltaTime)
{
    if (entity.hasGravity()) {
        moveHorizontally(move_front);
    } else {
        double distance = deltaTime * movement_speed;
        move(distance * move_front);
    }
}

void CameraModel::moveBackward(double deltaTime)
{
    if (entity.hasGravity()) {
        moveHorizontally(-move_front);
    } else {
        double distance = deltaTime * movement_speed;
        move(-distance * move_front);
    }
}
    
void CameraModel::moveLeft(double deltaTime)
{
    if (entity.hasGravity()) {
        moveHorizontally(-move_right);
    } else {
        double distance = deltaTime * movement_speed;
        move(-distance * move_right);
    }
}
    
void CameraModel::moveRight(double deltaTime)
{
    if (entity.hasGravity()) {
        moveHorizontally(move_right);
    } else {
        double distance = deltaTime * movement_speed;
        move(distance * move_right);
    }
}

void CameraModel::moveUp(double deltaTime)
{
    double distance = deltaTime * movement_speed;
    // std::cout << "distance:" << distance << " up:" << glm::to_string(up) << std::endl;
    if (entity.hasGravity()) { // XXX move into controller
        // std::cout << "Jumping\n";
        if (entity.isOnGround()) {
            entity.getVelocity().y = movement_speed * 1.5;
        }
        // entity.addVelocity(glm::dvec3(0, 0.5, 0));
    } else {
        move(distance * move_up);
    }
}

void CameraModel::moveDown(double deltaTime)
{
    double distance = deltaTime * movement_speed;
    move(-distance * move_up);
}

void CameraModel::gameTick(double deltaTime)
{
    if (entity.hasGravity()) {
        entity.gameTick(deltaTime);
    }
    // update();
    // if (entity.isOnGround()) {
    //     // entity.setVelocity(glm::dvec3(0,0,0));
    // }
}

// void CameraModel::setCenter(double x, double y, double z)
// {
//     center = glm::dvec3(x, y, z);
//     update();
// }

void CameraModel::rotate(double pitch_change, double yaw_change)
{
    yaw += yaw_change;
    pitch += pitch_change;
    
    if (pitch > 89) pitch = 89;
    if (pitch < -89) pitch = -89;
    
    update();
}

void CameraModel::update()
{
    glm::dvec3 cam_front_tmp;
    cam_front_tmp.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    cam_front_tmp.y = sin(glm::radians(pitch));
    cam_front_tmp.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    
    glm::dvec3 move_front_tmp;
    move_front_tmp.x = cos(glm::radians(yaw));
    move_front_tmp.y = 0;
    move_front_tmp.z = sin(glm::radians(yaw));
    
    std::unique_lock<std::mutex> lock(camera_mutex);
    cam_front = glm::normalize(cam_front_tmp);
    cam_right = glm::normalize(glm::cross(cam_front, world_up));
    cam_up    = glm::normalize(glm::cross(cam_right, cam_front));
    
    move_front = glm::normalize(move_front_tmp);
    move_right = glm::normalize(glm::cross(move_front, world_up));
    move_up    = world_up;
}


glm::mat4 CameraModel::getViewMatrix(double cx, double cy, double cz) {    
    std::unique_lock<std::mutex> lock(camera_mutex);
    glm::dvec3 center = glm::dvec3(cx, cy, cz);
    glm::dvec3 adjusted_pos = entity.getCameraPos() - center;
    return glm::lookAt(adjusted_pos, adjusted_pos + cam_front, cam_up);
}


// Good:
// mat4x4((1.000000, -0.000000, 0.000000, 0.000000), (0.000000, 0.707107, 0.707107, 0.000000), (-0.000000, -0.707107, 0.707107, 0.000000), (0.000000, -0.000000, -4.242641, 1.000000))
// Bad:
// mat4x4((nan, nan, 0.000000, 0.000000), (nan, nan, 0.707107, 0.000000), (nan, nan, 0.707107, 0.000000), (nan, nan, -4.242641, 1.000000))
// mat4x4((1.000000, -0.000000, 0.000000, 0.000000), (0.000000, 0.707107, 0.707107, 0.000000), (-0.000000, -0.707107, 0.707107, 0.000000), (0.000000, -0.000000, -4.242641, 1.000000))
