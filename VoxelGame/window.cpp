#include "gl_includes.hpp"
#include "window.hpp"
#include <stdio.h>
#include <iostream>
#include <glm/gtc/matrix_transform.hpp>


// Right now, only support one window.
// Can replace this with map and do lookup in callbacks;
static Window *global_window = 0;

static void global_mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
    global_window->mouse_callback(xpos, ypos);
}

static void global_key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    global_window->key_callback(key, scancode, action, mods);
}

static void global_framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    global_window->framebuffer_size_callback(width, height);
}

static void global_button_callback(GLFWwindow* window, int button, int action, int mods)
{
    global_window->button_callback(button, action, mods);
}

static void global_scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    global_window->scroll_callback(xoffset, yoffset);
}
    
void Window::create()
{
    GLFWwindow *window;

    /* Initialize the library */
    if (!glfwInit()) {
        fprintf(stderr, "Error doing glfwInit\n");
        return;
    }

 #ifdef __APPLE__
    /* We need to explicitly ask for a 3.3 context on OS X */
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
 #endif

    /* Create a windowed mode window and its OpenGL context */
    window = glfwCreateWindow(720, 720, "Hello World", NULL, NULL);
    if (!window) {
        fprintf(stderr, "Error creating window\n");
        return;
    }
    std::cout << "Window ptr: " << ((void*)window) << std::endl;

    GLFWmonitor* monitor = glfwGetPrimaryMonitor();
    monitor_ptr = monitor;

    /* Make the window's context current */
    glfwMakeContextCurrent(window);

#if defined _WIN32 || defined _WIN64
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return;
    }
#endif

    did_init = true;

    glfwGetFramebufferSize(window, &window_width, &window_height);
    // printf("width=%d, height=%d\n", w, h);
    glViewport(0, 0, window_width, window_height);
    
    int os_window_w, os_window_h;
    glfwGetWindowSize(window, &os_window_w, &os_window_h);
    cursor_scale_x = (double)window_width / (double)os_window_w;
    cursor_scale_y = (double)window_height / (double)os_window_h;

    glfwSetFramebufferSizeCallback(window, global_framebuffer_size_callback);  
    
    grab_cursor = true;
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwGetCursorPos(window, &last_mouse_x, &last_mouse_y);
    glfwSetCursorPosCallback(window, global_mouse_callback);
    glfwSetKeyCallback(window, global_key_callback);
    glfwSetMouseButtonCallback(window, global_button_callback);
    glfwSetScrollCallback(window, global_scroll_callback);
    glfwSwapInterval(1);
    
    if (glfwRawMouseMotionSupported())
        glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
        
    window_ptr = (void *)window;
}

void Window::toggleFullScreen()
{
    GLFWmonitor* monitor = (GLFWmonitor*)monitor_ptr;
    GLFWwindow* window = (GLFWwindow*)window_ptr;
    const GLFWvidmode* mode = glfwGetVideoMode(monitor);

    if (in_full_screen) {
        glfwSetWindowMonitor(window, 0, 100, 100, 800, 600, GLFW_DONT_CARE);
    } else {
        glfwSetWindowMonitor(window, monitor, 0, 0, mode->width, mode->height, GLFW_DONT_CARE);
    }
    in_full_screen = !in_full_screen;
}

Window::Window()
{
    window_ptr = 0;
    window_width = 0;
    window_height = 0;
    last_mouse_x = 0;
    last_mouse_y = 0;
    grab_cursor = false;
    did_init = false;
    in_full_screen = false;
    
    global_window = this;
    
    // create();
}

void Window::destroy()
{
    GLFWwindow *window = (GLFWwindow *)window_ptr;
    if (window) glfwDestroyWindow(window);
    if (did_init) glfwTerminate();
    window_ptr = 0;
    did_init = false;
}

Window::~Window()
{
    destroy();
}

void Window::framebuffer_size_callback(int width, int height)
{
    GLFWwindow *window = (GLFWwindow *)window_ptr;

    window_width = width;
    window_height = height;
    // printf("width=%d, height=%d\n", width, height);
    glViewport(0, 0, width, height);
    
    int os_window_w, os_window_h;
    glfwGetWindowSize(window, &os_window_w, &os_window_h);
    cursor_scale_x = (double)window_width / (double)os_window_w;
    cursor_scale_y = (double)window_height / (double)os_window_h;    
}

void Window::swap_buffers()
{
    GLFWwindow *window = (GLFWwindow *)window_ptr;
    glfwSwapBuffers(window);
}

bool Window::shouldClose()
{
    GLFWwindow *window = (GLFWwindow *)window_ptr;
    return !!glfwWindowShouldClose(window);
}

void Window::poll_events()
{
    glfwPollEvents();
}

glm::mat4 Window::getProjectionMatrix()
{
    return glm::perspective(fov, getAspectRatio(), 0.1, 300.0);
}
