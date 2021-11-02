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

enum
{
    GRAPHICS = 0,
    PRESENT,
};

typedef struct App
{
    MemAllocator allocator;
    GLFWwindow *window;
    IVec2 window_size;

    VkAllocationCallbacks *vkalloc;
    VkInstance inst;
    VkDebugUtilsMessengerEXT debug;
    VkSurfaceKHR surface;
    VkPhysicalDevice gpu;
    VkDevice device;
    VkSwapchainKHR swapchain;

    U32 queue_index[2];
    VkQueue queue[2];

    VkImageView image[4];
    U32 image_count;
} App;

static Cstr const layers[] = {"VK_LAYER_KHRONOS_validation"};

static U32 const frag_blob[] = {
#include "frag.spv"
};

static U32 const vert_blob[] = {
#include "vert.spv"
};

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
appDiagnostic(App *app, Cstr format, ...)
{
    CF_UNUSED(app);

    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
}

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

static VKAPI_ATTR VkBool32 VKAPI_CALL
appDebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT severity,
                 VkDebugUtilsMessageTypeFlagsEXT type,
                 const VkDebugUtilsMessengerCallbackDataEXT *callback_data, void *user_data)
{
    App *app = user_data;

    static const Cstr type_text[] = {
        [VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT] = "general",
        [VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT] = "validation",
        [VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT] = "performance",
    };

    static const Cstr severity_text[] = {
        [VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT] = "verbose",
        [VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT] = "info",
        [VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT] = "warning",
        [VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT] = "error",
    };

    appDiagnostic(app, "Vulkan %s %s: %s\n", type_text[type], severity_text[severity],
                  callback_data->pMessage);

    return VK_FALSE;
}

VkResult
vkCreateDebugUtilsMessengerEXT(VkInstance instance,
                               VkDebugUtilsMessengerCreateInfoEXT const *pCreateInfo,
                               VkAllocationCallbacks const *pAllocator,
                               VkDebugUtilsMessengerEXT *pDebugMessenger)
{
#if CF_DEBUG
    PFN_vkCreateDebugUtilsMessengerEXT func =
        (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance,
                                                                  "vkCreateDebugUtilsMessengerEXT");
    if (func) return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
    return VK_ERROR_EXTENSION_NOT_PRESENT;
#else
    CF_UNUSED(instance);
    CF_UNUSED(pCreateInfo);
    CF_UNUSED(pAllocator);
    CF_UNUSED(pDebugMessenger);
    return VK_SUCCESS;
#endif
}

void
vkDestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger,
                                VkAllocationCallbacks const *pAllocator)
{
#if CF_DEBUG
    PFN_vkDestroyDebugUtilsMessengerEXT func =
        (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
            instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func) func(instance, debugMessenger, pAllocator);
#else
    CF_UNUSED(instance);
    CF_UNUSED(pDebugMessenger);
    CF_UNUSED(pAllocator);
#endif
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
    app->queue_index[GRAPHICS] = U32_MAX;
    app->queue_index[PRESENT] = U32_MAX;

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

            if (app->queue_index[PRESENT] == U32_MAX && supported)
            {
                app->queue_index[PRESENT] = queue_index;
            }

            if (app->queue_index[GRAPHICS] == U32_MAX &&
                (queues[queue_index].queueFlags & VK_QUEUE_GRAPHICS_BIT))
            {
                app->queue_index[GRAPHICS] = queue_index;
            }
        }

        if (app->queue_index[GRAPHICS] != U32_MAX && app->queue_index[PRESENT] != U32_MAX)
        {
            app->gpu = gpus[gpu_index];
        }
    }

    memFreeArray(app->allocator, queues, queues_cap);
    memFreeArray(app->allocator, gpus, num_gpus);

    if (!app->gpu) appTerminate(app, "Cannot find a suitable GPU\n");
}

static void
appCreateLogicalDevice(App *app)
{
    F32 const queue_priority = 1.0f;
    U32 const queue_count = (app->queue_index[GRAPHICS] != app->queue_index[PRESENT] ? 2 : 1);

    VkDeviceQueueCreateInfo queue_info[2] = {
        [GRAPHICS] =
            {
                .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
                .queueFamilyIndex = app->queue_index[GRAPHICS],
                .queueCount = 1,
                .pQueuePriorities = &queue_priority,
            },
        [PRESENT] =
            {
                .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
                .queueFamilyIndex = app->queue_index[PRESENT],
                .queueCount = 1,
                .pQueuePriorities = &queue_priority,
            },
    };

    Cstr const device_extensions[] = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};

    VkDeviceCreateInfo device_info = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .ppEnabledExtensionNames = device_extensions,
        .enabledExtensionCount = CF_ARRAY_SIZE(device_extensions),
        .pQueueCreateInfos = queue_info,
        .queueCreateInfoCount = queue_count,
#if CF_DEBUG
        .ppEnabledLayerNames = layers,
        .enabledLayerCount = CF_ARRAY_SIZE(layers),
#endif
    };

    VkResult res = vkCreateDevice(app->gpu, &device_info, app->vkalloc, &app->device);
    appCheckResult(app, res);

    vkGetDeviceQueue(app->device, app->queue_index[GRAPHICS], 0, &app->queue[GRAPHICS]);
    vkGetDeviceQueue(app->device, app->queue_index[PRESENT], 0, &app->queue[PRESENT]);
}

static void
appCreateSwapchain(App *app)
{
    VkSurfaceCapabilitiesKHR caps;
    VkResult res = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(app->gpu, app->surface, &caps);
    appCheckResult(app, res);

    VkSwapchainCreateInfoKHR info = {
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface = app->surface,
        .oldSwapchain = VK_NULL_HANDLE, // TODO (Matteo): Handle swapchain update
        .imageArrayLayers = 1,
        .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        .preTransform = caps.currentTransform,
        .clipped = VK_TRUE,
        .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
    };

    // NOTE (Matteo): In case the present and graphics queues differ, we allow them to share the
    // swapchain images concurrently
    if (app->queue_index[GRAPHICS] != app->queue_index[PRESENT])
    {
        info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        info.pQueueFamilyIndices = app->queue_index;
        info.queueFamilyIndexCount = 2;
    }
    else
    {
        info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }

    // Retrieve best surface format
    {
        U32 num_formats;
        res = vkGetPhysicalDeviceSurfaceFormatsKHR(app->gpu, app->surface, &num_formats, NULL);
        appCheckResult(app, res);

        // TODO (Matteo): Use this check for GPU picking
        if (num_formats == 0) appTerminate(app, "GPU does not support swapchain\n");

        VkSurfaceFormatKHR *formats =
            memAllocArray(app->allocator, VkSurfaceFormatKHR, num_formats);
        vkGetPhysicalDeviceSurfaceFormatsKHR(app->gpu, app->surface, &num_formats, formats);

        U32 format_index = 0;

        for (U32 index = 0; index < num_formats; ++index)
        {
            if (formats[index].format == VK_FORMAT_B8G8R8A8_SRGB &&
                formats[index].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
            {
                format_index = index;
                break;
            }
        }

        info.imageFormat = formats[format_index].format;
        info.imageColorSpace = formats[format_index].colorSpace;

        memFreeArray(app->allocator, formats, num_formats);
    }

    // Retrieve best presentation mode
    info.presentMode = VK_PRESENT_MODE_FIFO_KHR;
    {
        U32 num_modes;

        res = vkGetPhysicalDeviceSurfacePresentModesKHR(app->gpu, app->surface, &num_modes, NULL);
        appCheckResult(app, res);

        // TODO (Matteo): Use this check for GPU picking
        if (num_modes == 0) appTerminate(app, "GPU does not support swapchain\n");

        VkPresentModeKHR *modes = memAllocArray(app->allocator, VkPresentModeKHR, num_modes);
        vkGetPhysicalDeviceSurfacePresentModesKHR(app->gpu, app->surface, &num_modes, modes);

        for (U32 index = 0; index < num_modes; ++index)
        {
            if (modes[index] == VK_PRESENT_MODE_MAILBOX_KHR)
            {
                info.presentMode = VK_PRESENT_MODE_MAILBOX_KHR;
                break;
            }
        }
        memFreeArray(app->allocator, modes, num_modes);
    }

    // Retrieve swapchain extent
    info.imageExtent = caps.currentExtent;
    if (info.imageExtent.width == U32_MAX)
    {
        // NOTE (Matteo): This means the current extent is unknown, so we must set it to the
        // framebuffer size
        CF_ASSERT(info.imageExtent.height == U32_MAX, "Inconsistent current extent");

        I32 width, height;
        glfwGetFramebufferSize(app->window, &width, &height);

        info.imageExtent.width =
            cfClamp((U32)width, caps.minImageExtent.width, caps.maxImageExtent.width);
        info.imageExtent.height =
            cfClamp((U32)height, caps.minImageExtent.height, caps.maxImageExtent.height);
    }

    // NOTE (Matteo): Request one image more than the minimum to avoid excessive waiting on the
    // driver
    info.minImageCount = caps.minImageCount + 1;
    if (caps.maxImageCount > 0 && info.minImageCount > caps.maxImageCount)
    {
        info.minImageCount = caps.maxImageCount;
    }

    res = vkCreateSwapchainKHR(app->device, &info, app->vkalloc, &app->swapchain);
    appCheckResult(app, res);

    // Create image views
    {
        res = vkGetSwapchainImagesKHR(app->device, app->swapchain, &app->image_count, NULL);
        appCheckResult(app, res);

        if (app->image_count > CF_ARRAY_SIZE(app->image))
        {
            appTerminate(app, "%u swap chain images requested but %llu available\n",
                         app->image_count, CF_ARRAY_SIZE(app->image));
        }

        VkImage images[CF_ARRAY_SIZE(app->image)];
        vkGetSwapchainImagesKHR(app->device, app->swapchain, &app->image_count, images);

        VkImageViewCreateInfo image_info = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = info.imageFormat,
            .components.r = VK_COMPONENT_SWIZZLE_IDENTITY,
            .components.g = VK_COMPONENT_SWIZZLE_IDENTITY,
            .components.b = VK_COMPONENT_SWIZZLE_IDENTITY,
            .components.a = VK_COMPONENT_SWIZZLE_IDENTITY,
            .subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .subresourceRange.baseMipLevel = 0,
            .subresourceRange.levelCount = 1,
            .subresourceRange.baseArrayLayer = 0,
            .subresourceRange.layerCount = 1,
        };

        for (U32 index = 0; index < app->image_count; ++index)
        {
            image_info.image = images[index];
            res = vkCreateImageView(app->device, &image_info, app->vkalloc, app->image + index);
            appCheckResult(app, res);
        }
    }
}

static VkShaderModule
appCreateShaderModule(App *app, U32 const *code, Usize code_size)
{
    VkShaderModule module;
    VkResult res = vkCreateShaderModule(app->device,
                                        &(VkShaderModuleCreateInfo){
                                            .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
                                            .pCode = code,
                                            .codeSize = sizeof(code_size),
                                        },
                                        app->vkalloc, &module);
    appCheckResult(app, res);
    return module;
}

static void
appCreatePipeline(App *app)
{
    VkShaderModule frag = appCreateShaderModule(app, frag_blob, sizeof(frag_blob));
    VkShaderModule vert = appCreateShaderModule(app, vert_blob, sizeof(vert_blob));

    VkPipelineShaderStageCreateInfo stage_info[2] = {
        {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .module = vert,
            .pName = "main",
            .stage = VK_SHADER_STAGE_VERTEX_BIT,
        },
        {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .module = frag,
            .pName = "main",
            .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
        },
    };

    VkGraphicsPipelineCreateInfo pipe_info = {
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .pStages = stage_info,
        .stageCount = 2,
    };

    CF_UNUSED(pipe_info);

    vkDestroyShaderModule(app->device, frag, app->vkalloc);
    vkDestroyShaderModule(app->device, vert, app->vkalloc);
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

    Cstr extensions[16] = {0};
    U32 num_extensions = 0;

    // Get the required extensions from GLFW
    Cstr const *glfw_extensions = glfwGetRequiredInstanceExtensions(&num_extensions);
    CF_ASSERT(num_extensions < CF_ARRAY_SIZE(extensions), "Too many required extensions");
    memCopy(glfw_extensions, extensions, num_extensions * sizeof(*extensions));

#if CF_DEBUG
    // Add debug layer
    inst_info.ppEnabledLayerNames = layers;
    inst_info.enabledLayerCount = CF_ARRAY_SIZE(layers);
    // Add debug extension
    extensions[num_extensions++] = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;
#endif

    inst_info.enabledExtensionCount = num_extensions;
    inst_info.ppEnabledExtensionNames = extensions;

    VkResult res = vkCreateInstance(&inst_info, app->vkalloc, &app->inst);
    appCheckResult(app, res);

    // Setup debugging
    // NOTE (Matteo): VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT ignored for now
    VkDebugUtilsMessengerCreateInfoEXT debug_info = {
        .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
        .messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                           VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                           VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
        .messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                       VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                       VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
        .pfnUserCallback = appDebugCallback,
        .pUserData = app,
    };
    res = vkCreateDebugUtilsMessengerEXT(app->inst, &debug_info, app->vkalloc, &app->debug);
    appCheckResult(app, res);

    // Retrieve a Vulkan surface from GLFW window
    res = glfwCreateWindowSurface(app->inst, app->window, app->vkalloc, &app->surface);
    appCheckResult(app, res);

    // Choose a suitable GPU for rendering
    appPickGpu(app);

    // Create a logical device with associated graphics and presentation queues
    appCreateLogicalDevice(app);

    appCreateSwapchain(app);

    appCreatePipeline(app);
}

void
appShutdown(App *app)
{
    for (U32 index = 0; index < app->image_count; ++index)
    {
        vkDestroyImageView(app->device, app->image[index], app->vkalloc);
    }

    vkDestroySwapchainKHR(app->device, app->swapchain, app->vkalloc);
    vkDestroyDevice(app->device, app->vkalloc);
    vkDestroySurfaceKHR(app->inst, app->surface, app->vkalloc);
    vkDestroyDebugUtilsMessengerEXT(app->inst, app->debug, app->vkalloc);
    vkDestroyInstance(app->inst, app->vkalloc);

    glfwDestroyWindow(app->window);
    glfwTerminate();
}

void
appMainLoop(App *app)
{
    while (!glfwWindowShouldClose(app->window))
    {
        glfwPollEvents();
    }
}
