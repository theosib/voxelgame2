#ifndef _INCLUDED_WINDOW
#define _INCLUDED_WINDOW

#include <glm/glm.hpp>

class Window {
protected:
    void *window_ptr, *monitor_ptr;
    int window_width, window_height;
    double last_mouse_x, last_mouse_y;
    bool grab_cursor, did_init;
    double fov;
    double cursor_scale_x, cursor_scale_y;
    bool in_full_screen;
        
public:
    Window();
    virtual ~Window();

    void framebuffer_size_callback(int width, int height);    
    void create();
    void destroy();
    void swap_buffers();
    void poll_events();

    void toggleFullScreen();
    
    virtual void process_input() = 0;
    virtual void key_callback(int key, int scancode, int action, int mods) = 0;
    virtual void mouse_callback(double xpos, double ypos) = 0;
    virtual void button_callback(int button, int action, int mods) = 0;
    virtual void scroll_callback(double xoffset, double yoffset) = 0;
    
    void next_frame() {
        swap_buffers();
        poll_events();
        process_input();
    }
    
    int getWidth() { return window_width; }
    int getHeight() { return window_height; }
    double getAspectRatio() {
        return (double)window_width / (double)window_height;
    }
    glm::mat4 getProjectionMatrix();
    void setFOV(double radians) {
        fov = radians;
    }
    
    bool valid() { return !!window_ptr; }
    bool shouldClose();
};

#endif