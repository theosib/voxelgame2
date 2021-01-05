#ifdef __APPLE__
#define GLFW_INCLUDE_GLCOREARB
#define GL_SILENCE_DEPRECATION
#endif
#if defined _WIN32 || defined _WIN64
#include "glad/glad.h"
#endif
#include <GLFW/glfw3.h>
