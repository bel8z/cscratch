#include "platform.h"

// Backend libraries
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>

// Foundation libraries
#include "foundation/error.h"
#include "foundation/memory.h"

// Standard libraries - TODO (Matteo): Get rid of them?
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

//-------------------//
//   App interface   //
//-------------------//

typedef struct App
{
    MemAllocator allocator;
    GLFWwindow *window;
    IVec2 window_size;

    VkAllocationCallbacks *vkalloc;
    VkInstance inst;
    VkSurfaceKHR surface;
    VkPhysicalDevice gpu;

    U32 graphics_queue;
    U32 present_queue;

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

CF_PRINTF_LIKE(1, 2)
static void
appTerminate(App *app, Cstr format, ...)
{
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);

    appShutdown(app);
    exit(EXIT_FAILURE);
}

static void
appCheckResult(App *app, VkResult result)
{
    if (result) appTerminate(app, "Vulkan error %d\n", result);
}

static void
appPickGpu(App *app)
{
    U32 num_gpus;
    VkResult res = vkEnumeratePhysicalDevices(app->inst, &num_gpus, NULL);
    appCheckResult(app, res);

    VkPhysicalDevice *gpus = memAllocArray(app->allocator, VkPhysicalDevice, num_gpus);
    vkEnumeratePhysicalDevices(app->inst, &num_gpus, gpus);

    VkQueueFamilyProperties *queues = NULL;
    U32 queues_cap = 0;

    app->gpu = VK_NULL_HANDLE;
    app->graphics_queue = U32_MAX;
    app->present_queue = U32_MAX;

    for (U32 gpu_index = 0; gpu_index < num_gpus; ++gpu_index)
    {
        U32 num_queues = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(gpus[gpu_index], &num_queues, NULL);

        if (num_queues > queues_cap)
        {
            queues = memReallocArray(app->allocator, queues, queues_cap, num_queues);
            queues_cap = num_queues;
        }

        vkGetPhysicalDeviceQueueFamilyProperties(gpus[gpu_index], &num_queues, queues);

        for (U32 queue_index = 0; queue_index < num_queues; ++queue_index)
        {
            VkBool32 supported = VK_FALSE;
            res = vkGetPhysicalDeviceSurfaceSupportKHR(gpus[gpu_index], queue_index, app->surface,
                                                       &supported);
            appCheckResult(app, res);

            app->present_queue = (app->present_queue == U32_MAX && supported);
            app->graphics_queue = (app->graphics_queue == U32_MAX &&
                                   (queues[queue_index].queueFlags & VK_QUEUE_GRAPHICS_BIT));
        }

        if (app->graphics_queue != U32_MAX && app->present_queue != U32_MAX)
        {
            app->gpu = gpus[gpu_index];
        }
    }

    memFreeArray(app->allocator, queues, queues_cap);
    memFreeArray(app->allocator, gpus, num_gpus);

    if (!app->gpu) appTerminate(app, "Cannot find a suitable GPU\n");
}

void
appInit(App *app, Platform *platform)
{
    app->allocator = platform->heap;
    app->vkalloc = NULL; // TODO (Matteo): Implement

    //=== Initialize GLFW and create window  ===//

    glfwInit();
    // NOTE (Matteo): Don't require OpenGL
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    // TODO (Matteo): Handle resizing
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    app->window_size = (IVec2){.width = 800, .height = 600};
    app->window = glfwCreateWindow(app->window_size.width, app->window_size.height, "Vulkan window",
                                   NULL, NULL);

    //=== Initialize Vukan  ===//

    VkInstanceCreateInfo inst_info = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pApplicationInfo =
            &(VkApplicationInfo){
                .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
                .apiVersion = VK_API_VERSION_1_0,
            },
    };

    // Get the required extensions from GLFW
    inst_info.ppEnabledExtensionNames =
        glfwGetRequiredInstanceExtensions(&inst_info.enabledExtensionCount);

    VkResult res = vkCreateInstance(&inst_info, app->vkalloc, &app->inst);
    appCheckResult(app, res);

    // Retrieve a Vulkan surface from GLFW window
    res = glfwCreateWindowSurface(app->inst, app->window, app->vkalloc, &app->surface);
    appCheckResult(app, res);

    // Choose a suitable GPU for rendering
    appPickGpu(app);
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
    vkDestroySurfaceKHR(app->inst, app->surface, app->vkalloc);
    vkDestroyInstance(app->inst, app->vkalloc);

    glfwDestroyWindow(app->window);
    glfwTerminate();
}
