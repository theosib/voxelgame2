#include "time.hpp"
#include "gl_includes.hpp"

double ref::currentTime()
{
    return glfwGetTime();
}
