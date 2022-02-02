#pragma once

#include "foundation/core.h"

typedef struct GLFWwindow GLFWwindow;

typedef enum GlfwClientApi
{
    GlfwClientApi_Unknown,
    GlfwClientApi_OpenGL,
    GlfwClientApi_Vulkan
} GlfwClientApi;

// IMPORTANT (Matteo): Call this AFTER installing your own callbacks, otherwise you will overwrite
// the ones installed by the backend
CF_API bool guiGlfwInit(GLFWwindow *window, GlfwClientApi client_api);
CF_API void guiGlfwShutdown();
CF_API void guiGlfwNewFrame();
