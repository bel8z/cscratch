#include "platform.h"

// Backend libraries
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>

// Foundation libraries
#include "foundation/error.h"
#include "foundation/memory.h"

// Standard libraries - TODO (Matteo): Get rid of them?
#include <stdio.h>

//-------------------//
//   App interface   //
//-------------------//

typedef struct App
{
    MemAllocator allocator;
    GLFWwindow *window;
    IVec2 window_size;
} App;

static void appInit(App *app, Platform *platform);
static void appShutdown(App *app);
static void appMainLoop(App *app);

//-----------------//
//   Entry point   //
//-----------------//

// TODO (Matteo): Review of the platform layer for graphic apps

#if CF_OS_WIN32
#    include "platform_win32.c"
#else
#    error "OS specific layer not implemented"
#endif

I32
platformMain(Platform *platform, CommandLine *cmd_line)
{
    CF_UNUSED(platform);
    CF_UNUSED(cmd_line);

    App app = {0};
    appInit(&app, platform);
    appMainLoop(&app);
    appShutdown(&app);

    return 0;
}

//---------------//
//   App logic   //
//---------------//

static void
testPrintVulkanExtensions(MemAllocator allocator)
{
    U32 num_extensions = 0;
    vkEnumerateInstanceExtensionProperties(NULL, &num_extensions, NULL);
    VkExtensionProperties *extensions =
        memAllocArray(allocator, VkExtensionProperties, num_extensions);
    vkEnumerateInstanceExtensionProperties(NULL, &num_extensions, extensions);

    fprintf(stderr, "#%u Vulkan extensions supported:\n", num_extensions);

    for (U32 index = 0; index < num_extensions; ++index)
    {
        VkExtensionProperties *prop = extensions + index;
        fprintf(stderr, "- %s (Spec version %u)\n", prop->extensionName, prop->specVersion);
    }
    memFreeArray(allocator, extensions, num_extensions);
}

void
appInit(App *app, Platform *platform)
{
    app->allocator = platform->heap;

    testPrintVulkanExtensions(app->allocator);

    glfwInit();

    // NOTE (Matteo): Don't require OpenGL
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    // TODO (Matteo): Handle resizing
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    app->window_size = (IVec2){.width = 800, .height = 600};
    app->window = glfwCreateWindow(app->window_size.width, app->window_size.height, "Vulkan window",
                                   NULL, NULL);
}

void
appMainLoop(App *app)
{
    while (!glfwWindowShouldClose(app->window))
    {
        glfwPollEvents();
    }
}

void
appShutdown(App *app)
{
    glfwDestroyWindow(app->window);
    glfwTerminate();
}
