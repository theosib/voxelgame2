#include "gl_includes.hpp"
#include "gamewindow.hpp"
#include <iostream>

GameWindow::GameWindow()
{
    last_key_movement = 0;
    showing_menu = false;
    scroll_dir = 0;
    right_button = false;
    left_button = false;
}

GameWindow::~GameWindow() {}

void GameWindow::process_input()
{
    GLFWwindow *window = (GLFWwindow *)window_ptr;
    
    bool w = glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS;
    bool s = glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS;
    bool a = glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS;
    bool d = glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS;
    bool space = glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS;
    bool shift = glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS;
    
    double deltaTime = 0;
    double now = glfwGetTime();
    if (!w && !s && !a && !d && !shift && !space) {
        last_key_movement = -1;
    } else {
        if (last_key_movement < 0) last_key_movement = now;
        deltaTime = now - last_key_movement;
        // std::cout << "deltaTime=" << deltaTime << std::endl;
        last_key_movement = now;
        if (deltaTime < 0.008) deltaTime = 0.008;
    }
    
    if (w) camera->moveForward(deltaTime);
    if (s) camera->moveBackward(deltaTime);
    if (a) camera->moveLeft(deltaTime);
    if (d) camera->moveRight(deltaTime);
    if (shift) camera->moveDown(deltaTime);
    if (space) camera->moveUp(deltaTime);
}

void GameWindow::key_callback(int key, int scancode, int action, int mods)
{
    GLFWwindow *window = (GLFWwindow *)window_ptr;
    
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        if (showing_menu) {
            showing_menu = false;
            grab_cursor = true;
        } else {
            grab_cursor = !grab_cursor;
        }
    }

    if (key == GLFW_KEY_Q && action == GLFW_PRESS && (mods & GLFW_MOD_CONTROL)) {
        glfwSetWindowShouldClose(window, true);
    }
    
    if (key == GLFW_KEY_E && action == GLFW_PRESS) {
        showing_menu = !showing_menu;
        grab_cursor = !showing_menu;
    }
    
    if (key == GLFW_KEY_F && action == GLFW_PRESS) {
        camera->toggleFly();
    }

    if (key == GLFW_KEY_F11 && action == GLFW_PRESS) {
        toggleFullScreen();
    }

    glfwSetInputMode(window, GLFW_CURSOR, grab_cursor ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
}

void GameWindow::scroll_callback(double xoffset, double yoffset)
{
    if (yoffset < 0) {
        scroll_dir = -1;
    } else if (yoffset > 0) {
        scroll_dir = 1;
    }
}


void GameWindow::showMenu(bool show)
{
    if (showing_menu == show) return;
    GLFWwindow *window = (GLFWwindow *)window_ptr;
    showing_menu = show;
    grab_cursor = !showing_menu;
    glfwSetInputMode(window, GLFW_CURSOR, grab_cursor ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
}

void GameWindow::mouse_callback(double xpos, double ypos)
{
    GLFWwindow *window = (GLFWwindow *)window_ptr;
    
    double xoffset = xpos - last_mouse_x;
    double yoffset = last_mouse_y - ypos;
    
    last_mouse_x = xpos;
    last_mouse_y = ypos;
    
    if (!grab_cursor) return;
    camera->mouseMovement(xoffset, yoffset);
    
    // std::cout << "x:" << xpos << " y:" << ypos << std::endl;
}

void GameWindow::button_callback(int button, int action, int mods)
{
    if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS) right_button = true;
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) left_button = true;
}

void GameWindow::getCursorPos(double *x, double *y)
{
    GLFWwindow *window = (GLFWwindow *)window_ptr;
    glfwGetCursorPos(window, x, y);
    *x *= cursor_scale_x;
    *y *= cursor_scale_y;
}
