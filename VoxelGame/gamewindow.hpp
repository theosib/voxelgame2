#ifndef _INCLUDED_GAME_WINDOW
#define _INCLUDED_GAME_WINDOW

#include "window.hpp"
#include "cameracontroller.hpp"
#include "shader.hpp"

class GameWindow : public Window {
private:
    CameraController *camera;
    double last_key_movement;
    bool right_button, left_button;
    bool showing_menu;
    int scroll_dir;
    
public:
    GameWindow();
    virtual ~GameWindow();
    
    virtual void process_input();
    virtual void key_callback(int key, int scancode, int action, int mods);
    virtual void mouse_callback(double xpos, double ypos);
    virtual void button_callback(int button, int action, int mods);
    virtual void scroll_callback(double xoffset, double yoffset);
    
    void setCamera(CameraController *cam) {
        camera = cam;
    }
    
    bool checkRightButton() { return right_button; }
    bool checkLeftButton() { return left_button; }
    bool isShowingMenu() { return showing_menu; }
    void showMenu(bool show);
    int getScrollDir() { return scroll_dir; }
    
    void clearRightButton() { right_button = false; }
    void clearLeftButton() { left_button = false; }
    void clearScrollDir() { scroll_dir = 0; }
    
    void getCursorPos(double *x, double *y);
};

#endif