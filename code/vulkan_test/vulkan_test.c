#include "platform.h"

// Gui library
#include "gui/gui.h"

// Backend libraries
#define GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>

// Foundation libraries
#include "foundation/error.h"
#include "foundation/memory.h"
#include "foundation/strings.h"
#include "foundation/time.h"

// Standard libraries - TODO (Matteo): Get rid of them?
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

//-------------------//
//   Configuration   //
//-------------------//

#define RENDER_GUI false

#define BLEND_ENABLED false

#define PREFERRED_PRESENT_MODE ((VkPresentModeKHR)VK_PRESENT_MODE_MAILBOX_KHR)

// TODO (Matteo): Can this be fixed? Separate render passes for GUI and app?
// In order to render IMGUI inside the main render pass, the format must be
// VK_FORMAT_B8G8R8A8_UNORM.
#if RENDER_GUI
#    define PREFERRED_FORMAT VK_FORMAT_B8G8R8A8_UNORM
#else
#    define PREFERRED_FORMAT VK_FORMAT_B8G8R8A8_SRGB
#endif

// Configure the pipeline state that can change dynamically
static VkDynamicState const DYNAMIC_PIPELINE_STATES[] = {
    // VK_DYNAMIC_STATE_VIEWPORT,
    VK_DYNAMIC_STATE_LINE_WIDTH,
};

//----------//
//   Data   //
//----------//

enum QueueType
{
    GRAPHICS = 0,
    PRESENT,
};

typedef struct Vertex
{
    Vec3 pos;
    Vec3 color; // TODO (Matteo): Use Rgba colors
} Vertex;

typedef struct UniformBufferObject
{
    Mat4 model;
    Mat4 view;
    Mat4 proj;
} UniformBufferObject;

typedef struct Swapchain
{
    VkSwapchainKHR handle;
    VkExtent2D extent;
    VkFormat format;

    U32 image_count;
    VkImageView image[3];
    VkFramebuffer frame[3];
    VkFence fence[3];
} Swapchain;

typedef struct ResourceBuffer
{
    // TODO (Matteo): Better memory management for resources
    VkBuffer buffer;
    VkDeviceMemory memory;
    VkDeviceSize size;
    void *map;
} ResourceBuffer;

typedef struct Frame
{
    VkCommandPool cmd_pool;
    VkCommandBuffer cmd;

    VkFence fence;

    VkSemaphore image_available;
    VkSemaphore render_finished;

    VkDescriptorSet desc;
    ResourceBuffer uniform;
} Frame;

typedef struct App
{
    Platform *platform;

    Clock clock;

    MemAllocator allocator;
    GLFWwindow *window;

    VkAllocationCallbacks *vkalloc;
    VkInstance inst;
    VkDebugUtilsMessengerEXT debug;
    VkSurfaceKHR surface;
    VkPhysicalDevice gpu;

    VkDevice device;
    VkQueue queue[2];
    U32 queue_index[2];

    Swapchain swapchain;
    bool rebuild_swapchain;

    VkRenderPass render_pass;
    VkDescriptorSetLayout desc_layout;
    VkPipelineLayout pipe_layout;
    VkPipeline pipe;

    Frame frames[2];
    Usize frame_index;

    ResourceBuffer vertex;
    ResourceBuffer index;

    VkDescriptorPool desc_pool;

    bool gui_setup;

    UniformBufferObject ubo;
} App;

const Vertex g_vertices[] = {
    {.pos = {{-0.5f, -0.5f, 0.0f}}, .color = {{1.0f, 0.0f, 0.0f}}},
    {.pos = {{0.5f, -0.5f, 0.0f}}, .color = {{0.0f, 1.0f, 0.0f}}},
    {.pos = {{0.5f, 0.5f, 0.0f}}, .color = {{0.0f, 0.0f, 1.0f}}},
    {.pos = {{-0.5f, 0.5f, 0.0f}}, .color = {{1.0f, 1.0f, 1.0f}}},
    // {.pos = {{0.0f, 0.0f, 1.0f}}, .color = {{1.0f, 1.0f, 1.0f}}},
};

const U16 g_indices[] = {
    0, 1, 2, 2, 3, 0, //
    // 0, 4, 1, 3, 4, 2, //
};

//-----------------//
//   Entry point   //
//-----------------//

static void appInit(App *app, Platform *platform);
static void appShutdown(App *app);
static void appMainLoop(App *app);

// TODO (Matteo): Review of the platform layer for graphic apps

#if CF_OS_WIN32
#    include "platform_win32.c"
#else
#    error "OS specific layer not implemented"
#endif

I32
platformMain(Platform *platform, CommandLine *cmd_line)
{
    CF_UNUSED(cmd_line);
    App app = {0};
    appInit(&app, platform);
    appMainLoop(&app);
    appShutdown(&app);

    return 0;
}

//-----------------//
//   Gui backend   //
//-----------------//

typedef struct ImGui_ImplVulkan_InitInfo
{
    VkInstance Instance;
    VkPhysicalDevice PhysicalDevice;
    VkDevice Device;
    uint32_t QueueFamily;
    VkQueue Queue;
    VkPipelineCache PipelineCache;
    VkDescriptorPool DescriptorPool;
    uint32_t Subpass;
    uint32_t MinImageCount;            // >= 2
    uint32_t ImageCount;               // >= MinImageCount
    VkSampleCountFlagBits MSAASamples; // >= VK_SAMPLE_COUNT_1_BIT
    const VkAllocationCallbacks *Allocator;
    void (*CheckVkResultFn)(VkResult err);
} ImGui_ImplVulkan_InitInfo;

extern bool ImGui_ImplVulkan_Init(ImGui_ImplVulkan_InitInfo *info, VkRenderPass render_pass);
extern void ImGui_ImplVulkan_Shutdown();
extern void ImGui_ImplVulkan_NewFrame();
extern void ImGui_ImplVulkan_RenderDrawData(ImDrawData *draw_data, VkCommandBuffer command_buffer,
                                            VkPipeline pipeline);
extern bool ImGui_ImplVulkan_CreateFontsTexture(VkCommandBuffer command_buffer);
extern void ImGui_ImplVulkan_DestroyFontUploadObjects();
extern void ImGui_ImplVulkan_SetMinImageCount(
    uint32_t min_image_count); // To override MinImageCount after initialization (e.g. if swap chain
                               // is recreated)

extern bool ImGui_ImplGlfw_InitForVulkan(GLFWwindow *window, bool install_callbacks);
extern void ImGui_ImplGlfw_Shutdown();
extern void ImGui_ImplGlfw_NewFrame();

//------------------------//
//   Vulkan debug layer   //
//------------------------//

// Vulkan debug layers
static Cstr const g_layers[] = {
    "VK_LAYER_KHRONOS_validation",
};

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

//---------------------//
//   App diagnostics   //
//---------------------//

#define VK_CHECK(res) appCheckResult(app, res)

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

    App *app = user_data;
    appDiagnostic(app, "Vulkan %s %s: %s\n", type_text[type], severity_text[severity],
                  callback_data->pMessage);

    return VK_FALSE;
}

static void
guiCheckResult(VkResult result)
{
    App *app = guiUserData();
    appCheckResult(app, result);
}

//--------------------------------//
//   Initialization / Shutdown    //
//--------------------------------//

static inline VkExtent2D
extentClamp(VkExtent2D value, VkExtent2D min, VkExtent2D max)
{
    value.width = cfClamp(value.width, min.width, max.width);
    value.height = cfClamp(value.height, min.height, max.height);
    return value;
}

static VkExtent2D
appGetFrameSize(App *app)
{
    I32 width, height;
    glfwGetFramebufferSize(app->window, &width, &height);
    return (VkExtent2D){.height = (U32)height, .width = (U32)width};
}

static void
appPickGpu(App *app)
{
    U32 num_gpus;
    VK_CHECK(vkEnumeratePhysicalDevices(app->inst, &num_gpus, NULL));

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
            VK_CHECK(vkGetPhysicalDeviceSurfaceSupportKHR(gpus[gpu_index], queue_index,
                                                          app->surface, &supported));

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
appPrintGpuMemoryProperties(App *app)
{
    VkPhysicalDeviceMemoryProperties props;
    vkGetPhysicalDeviceMemoryProperties(app->gpu, &props);

    appDiagnostic(app, "%u memory heaps:\n", props.memoryHeapCount);
    for (U32 index = 0; index < props.memoryHeapCount; ++index)
    {
        VkMemoryHeap *heap = props.memoryHeaps + index;
        appDiagnostic(app, "Heap #%u\n", index);
        appDiagnostic(app, "\tSize: %llu\n", heap->size);
        appDiagnostic(app, "\tFlags: ");

        if (heap->flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT)
        {
            appDiagnostic(app, "VK_MEMORY_HEAP_DEVICE_LOCAL_BIT, ");
        }

        if (heap->flags & VK_MEMORY_HEAP_MULTI_INSTANCE_BIT)
        {
            appDiagnostic(app, "VK_MEMORY_HEAP_MULTI_INSTANCE_BIT, ");
        }
        appDiagnostic(app, "\n");
    }

    appDiagnostic(app, "%u memory types:\n", props.memoryTypeCount);
    for (U32 index = 0; index < props.memoryTypeCount; ++index)
    {
        VkMemoryType *type = props.memoryTypes + index;
        appDiagnostic(app, "Type #%u\n", index);
        appDiagnostic(app, "\tHeap: %u\n", type->heapIndex);
        appDiagnostic(app, "\tFlags: ");

        if (type->propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
        {
            appDiagnostic(app, "VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, ");
        }
        if (type->propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)
        {
            appDiagnostic(app, "VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, ");
        }
        if (type->propertyFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)
        {
            appDiagnostic(app, "VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, ");
        }
        if (type->propertyFlags & VK_MEMORY_PROPERTY_HOST_CACHED_BIT)
        {
            appDiagnostic(app, "VK_MEMORY_PROPERTY_HOST_CACHED_BIT, ");
        }
        if (type->propertyFlags & VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT)
        {
            appDiagnostic(app, "VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT, ");
        }
        if (type->propertyFlags & VK_MEMORY_PROPERTY_PROTECTED_BIT)
        {
            appDiagnostic(app, "VK_MEMORY_PROPERTY_PROTECTED_BIT, ");
        }
        if (type->propertyFlags & VK_MEMORY_PROPERTY_DEVICE_COHERENT_BIT_AMD)
        {
            appDiagnostic(app, "VK_MEMORY_PROPERTY_DEVICE_COHERENT_BIT_AMD, ");
        }
        if (type->propertyFlags & VK_MEMORY_PROPERTY_DEVICE_UNCACHED_BIT_AMD)
        {
            appDiagnostic(app, "VK_MEMORY_PROPERTY_DEVICE_UNCACHED_BIT_AMD, ");
        }
        if (type->propertyFlags & VK_MEMORY_PROPERTY_RDMA_CAPABLE_BIT_NV)
        {
            appDiagnostic(app, "VK_MEMORY_PROPERTY_RDMA_CAPABLE_BIT_NV, ");
        }
        appDiagnostic(app, "\n");
    }
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
        .ppEnabledLayerNames = g_layers,
        .enabledLayerCount = CF_ARRAY_SIZE(g_layers),
#endif
    };

    VK_CHECK(vkCreateDevice(app->gpu, &device_info, app->vkalloc, &app->device));

    vkGetDeviceQueue(app->device, app->queue_index[GRAPHICS], 0, &app->queue[GRAPHICS]);
    vkGetDeviceQueue(app->device, app->queue_index[PRESENT], 0, &app->queue[PRESENT]);
}

static void
appCreateSwapchain(App *app)
{
    Swapchain *swapchain = &app->swapchain;

    VkSurfaceCapabilitiesKHR caps;
    VK_CHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(app->gpu, app->surface, &caps));

    // Retrieve swapchain extent
    swapchain->extent = caps.currentExtent;
    if (swapchain->extent.width == U32_MAX)
    {
        // NOTE (Matteo): This means the current extent is unknown, so we must set it to the
        // framebuffer size
        CF_ASSERT(swapchain->extent.height == U32_MAX, "Inconsistent current extent");

        swapchain->extent =
            extentClamp(appGetFrameSize(app), caps.minImageExtent, caps.maxImageExtent);
    }

    // Prepare swapchain parameters
    VkSwapchainCreateInfoKHR info = {
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface = app->surface,
        .oldSwapchain = VK_NULL_HANDLE, // TODO (Matteo): Handle swapchain update
        .imageArrayLayers = 1,
        .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        .imageExtent = swapchain->extent,
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
        VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(app->gpu, app->surface, &num_formats, NULL));

        // TODO (Matteo): Use this check for GPU picking
        if (num_formats == 0) appTerminate(app, "GPU does not support swapchain\n");

        VkSurfaceFormatKHR *formats =
            memAllocArray(app->allocator, VkSurfaceFormatKHR, num_formats);
        vkGetPhysicalDeviceSurfaceFormatsKHR(app->gpu, app->surface, &num_formats, formats);

        U32 format_index = 0;

        for (U32 index = 0; index < num_formats; ++index)
        {
            if (formats[index].format == PREFERRED_FORMAT &&
                formats[index].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
            {
                format_index = index;
                break;
            }
        }

        info.imageFormat = formats[format_index].format;
        info.imageColorSpace = formats[format_index].colorSpace;

        memFreeArray(app->allocator, formats, num_formats);

        swapchain->format = info.imageFormat;
    }

    // Retrieve best presentation mode
    info.presentMode = VK_PRESENT_MODE_FIFO_KHR;
    {
        U32 num_modes;

        VK_CHECK(
            vkGetPhysicalDeviceSurfacePresentModesKHR(app->gpu, app->surface, &num_modes, NULL));

        // TODO (Matteo): Use this check for GPU picking
        if (num_modes == 0) appTerminate(app, "GPU does not support swapchain\n");

        VkPresentModeKHR *modes = memAllocArray(app->allocator, VkPresentModeKHR, num_modes);
        vkGetPhysicalDeviceSurfacePresentModesKHR(app->gpu, app->surface, &num_modes, modes);

        for (U32 index = 0; index < num_modes; ++index)
        {
            if (modes[index] == PREFERRED_PRESENT_MODE)
            {
                info.presentMode = PREFERRED_PRESENT_MODE;
                break;
            }
        }
        memFreeArray(app->allocator, modes, num_modes);
    }

    // NOTE (Matteo): Request one image more than the minimum to avoid excessive waiting on the
    // driver
    info.minImageCount = caps.minImageCount + 1;
    if (caps.maxImageCount > 0 && info.minImageCount > caps.maxImageCount)
    {
        info.minImageCount = caps.maxImageCount;
    }

    VK_CHECK(vkCreateSwapchainKHR(app->device, &info, app->vkalloc, &swapchain->handle));
}

static void
appCreateRenderPass(App *app)
{
    VkAttachmentDescription color = {
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .format = app->swapchain.format,
        // Clear before rendering
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        // Preserve color for presentation
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        // TODO (Matteo): Revisit - Stencil is unused for now
        .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        // Don't care about the initial layout of the image
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        // The final layout is meant for presentation
        .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,

    };

    // Every subpass references one or more of the attachments that we've described using the
    // previous structure. In this case the color attachment is meant to be drawn upon during the
    // render pass, so its layout must transition to COLOR_ATTACHMENT_OPTIMAL.
    VkAttachmentReference color_ref = {
        .attachment = 0, // layout(location = 0) in the shader
        .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    };

    VkSubpassDescription subpass = {
        .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
        .pColorAttachments = &color_ref,
        .colorAttachmentCount = 1,
    };

    VkSubpassDependency dependency = {
        // Synchronize the implicit subpass before our subpass
        .srcSubpass = VK_SUBPASS_EXTERNAL,
        .dstSubpass = 0,
        // Wait the swapchain to finish reading from the image before to start writing onto it.
        // The operations are in the color attachment stage.
        .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .srcAccessMask = 0,
        .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
    };

    VkRenderPassCreateInfo info = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .pAttachments = &color,
        .attachmentCount = 1,
        .pSubpasses = &subpass,
        .subpassCount = 1,
        .pDependencies = &dependency,
        .dependencyCount = 1,
    };

    VK_CHECK(vkCreateRenderPass(app->device, &info, app->vkalloc, &app->render_pass));
}

static void
appCreateDescriptorSetLayout(App *app)
{
    // NOTE (Matteo): It is possible for the shader variable to represent an array of uniform
    // buffer objects, and 'descriptorCount' specifies the number of values in the array. This could
    // be used to specify a transformation for each of the bones in a skeleton for skeletal
    // animation, for example.
    VkDescriptorSetLayoutBinding ubo_binding = {
        .binding = 0,
        .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .descriptorCount = 1,
        .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
        .pImmutableSamplers = NULL, // TODO (Matteo): Revisit
    };

    VkDescriptorSetLayoutCreateInfo layout_info = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .bindingCount = 1,
        .pBindings = &ubo_binding,
    };

    VK_CHECK(
        vkCreateDescriptorSetLayout(app->device, &layout_info, app->vkalloc, &app->desc_layout));
}

static VkShaderModule
appCreateShaderModule(App *app, U32 const *code, Usize code_size)
{
    VkShaderModule module;
    VK_CHECK(vkCreateShaderModule(app->device,
                                  &(VkShaderModuleCreateInfo){
                                      .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
                                      .pCode = code,
                                      .codeSize = code_size,
                                  },
                                  app->vkalloc, &module));
    return module;
}

static void
appCreatePipeline(App *app)
{
    static U32 const frag_code[] = {
#include "frag.spv"
    };

    static U32 const vert_code[] = {
#include "vert.spv"
    };

    VkPipelineShaderStageCreateInfo stage_info[2] = {
        {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .module = appCreateShaderModule(app, vert_code, sizeof(vert_code)),
            .pName = "main",
            .stage = VK_SHADER_STAGE_VERTEX_BIT,
        },
        {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .module = appCreateShaderModule(app, frag_code, sizeof(frag_code)),
            .pName = "main",
            .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
        },
    };

    VkVertexInputBindingDescription vertex_binding = {
        .inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
        .stride = sizeof(Vertex),
    };

    VkVertexInputAttributeDescription vertex_attributes[] = {
        {.location = 0, .format = VK_FORMAT_R32G32B32_SFLOAT},
        {.location = 1, .format = VK_FORMAT_R32G32B32_SFLOAT, .offset = offsetof(Vertex, color)},
    };

    VkPipelineVertexInputStateCreateInfo input_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .pVertexBindingDescriptions = &vertex_binding,
        .vertexBindingDescriptionCount = 1,
        .pVertexAttributeDescriptions = vertex_attributes,
        .vertexAttributeDescriptionCount = CF_ARRAY_SIZE(vertex_attributes),
    };

    VkPipelineInputAssemblyStateCreateInfo input_assembly = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
        .primitiveRestartEnable = false,
    };

    VkPipelineViewportStateCreateInfo viewport_state = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .pViewports = &(VkViewport){.x = 0,
                                    .y = 0,
                                    .width = (F32)app->swapchain.extent.width,
                                    .height = (F32)app->swapchain.extent.height,
                                    .minDepth = 0,
                                    .maxDepth = 1},
        .viewportCount = 1,
        .pScissors = &(VkRect2D){.offset = {0, 0}, .extent = app->swapchain.extent},
        .scissorCount = 1,
    };

    VkPipelineRasterizationStateCreateInfo rasterizer = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .depthClampEnable = false,
        .rasterizerDiscardEnable = false, // Required if depthClampEnable = true
        .polygonMode = VK_POLYGON_MODE_FILL,
        .lineWidth = 1.0f, // Required if polygon mode is not FILL
        .cullMode = VK_CULL_MODE_NONE,
        // .cullMode = VK_CULL_MODE_BACK_BIT,
        // .frontFace = VK_FRONT_FACE_CLOCKWISE,
    };

    // TODO (Matteo): Revisit - For now multisampling is left disabled
    VkPipelineMultisampleStateCreateInfo multisampling = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .sampleShadingEnable = VK_FALSE,
        .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
    };

    VkPipelineColorBlendAttachmentState blend_attachment = {
        .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                          VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
#if !BLEND_ENABLED
        // Blending disabled
        .blendEnable = VK_FALSE,
        .srcColorBlendFactor = VK_BLEND_FACTOR_ONE,  // Optional
        .dstColorBlendFactor = VK_BLEND_FACTOR_ZERO, // Optional
        .colorBlendOp = VK_BLEND_OP_ADD,             // Optional
        .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,  // Optional
        .dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO, // Optional
        .alphaBlendOp = VK_BLEND_OP_ADD,             // Optional
#else
        // Alpha blending
        .blendEnable = VK_TRUE,
        .srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
        .dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
        .colorBlendOp = VK_BLEND_OP_ADD,
        .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
        .dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
        .alphaBlendOp = VK_BLEND_OP_ADD,
#endif
    };

    VkPipelineColorBlendStateCreateInfo blend = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .logicOpEnable = VK_FALSE,   // VK_TRUE for bitwise blending
        .logicOp = VK_LOGIC_OP_COPY, // Optional
        .attachmentCount = 1,
        .pAttachments = &blend_attachment,
        .blendConstants[0] = 0.0f, // Optional
        .blendConstants[1] = 0.0f, // Optional
        .blendConstants[2] = 0.0f, // Optional
        .blendConstants[3] = 0.0f, // Optional
    };

    VkPipelineDynamicStateCreateInfo dynamic_state = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .dynamicStateCount = CF_ARRAY_SIZE(DYNAMIC_PIPELINE_STATES),
        .pDynamicStates = DYNAMIC_PIPELINE_STATES,
    };

    // TODO (Matteo): Revisit - Creating an empty layout for now
    VkPipelineLayoutCreateInfo layout_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount = 1,
        .pSetLayouts = &app->desc_layout,
    };

    VK_CHECK(vkCreatePipelineLayout(app->device, &layout_info, app->vkalloc, &app->pipe_layout));

    VkGraphicsPipelineCreateInfo pipe_info = {
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .pStages = stage_info,
        .stageCount = 2,
        .pVertexInputState = &input_info,
        .pInputAssemblyState = &input_assembly,
        .pViewportState = &viewport_state,
        .pRasterizationState = &rasterizer,
        .pMultisampleState = &multisampling,
        .pDepthStencilState = NULL, // TODO (Matteo): Revisit
        .pColorBlendState = &blend,
        .pDynamicState = &dynamic_state,
        .layout = app->pipe_layout,
        .renderPass = app->render_pass,
        .subpass = 0,
    };

    VK_CHECK(vkCreateGraphicsPipelines(app->device, VK_NULL_HANDLE, 1, &pipe_info, app->vkalloc,
                                       &app->pipe));

    vkDestroyShaderModule(app->device, stage_info[0].module, app->vkalloc);
    vkDestroyShaderModule(app->device, stage_info[1].module, app->vkalloc);
}

static void
appCreateFrameBuffers(App *app)
{
    Swapchain *swapchain = &app->swapchain;

    VK_CHECK(
        vkGetSwapchainImagesKHR(app->device, swapchain->handle, &swapchain->image_count, NULL));

    if (swapchain->image_count > CF_ARRAY_SIZE(swapchain->image))
    {
        appTerminate(app, "%u swap chain images requested but %llu available\n",
                     swapchain->image_count, CF_ARRAY_SIZE(swapchain->image));
    }

    VkImage images[CF_ARRAY_SIZE(swapchain->image)];
    vkGetSwapchainImagesKHR(app->device, swapchain->handle, &swapchain->image_count, images);

    VkImageViewCreateInfo image_info = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = swapchain->format,
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

    VkFramebufferCreateInfo frame_info = {
        .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
        .renderPass = app->render_pass,
        .attachmentCount = 1,
        .width = swapchain->extent.width,
        .height = swapchain->extent.height,
        .layers = 1,
    };

    for (U32 index = 0; index < swapchain->image_count; ++index)
    {
        image_info.image = images[index];
        VK_CHECK(
            vkCreateImageView(app->device, &image_info, app->vkalloc, swapchain->image + index));

        frame_info.pAttachments = swapchain->image + index;
        VK_CHECK(
            vkCreateFramebuffer(app->device, &frame_info, app->vkalloc, swapchain->frame + index));
    }
}

static void
appCleanupSwapchain(App *app)
{
    vkDestroyPipeline(app->device, app->pipe, app->vkalloc);
    vkDestroyPipelineLayout(app->device, app->pipe_layout, app->vkalloc);

    for (U32 index = 0; index < app->swapchain.image_count; ++index)
    {
        vkDestroyFramebuffer(app->device, app->swapchain.frame[index], app->vkalloc);
        vkDestroyImageView(app->device, app->swapchain.image[index], app->vkalloc);
    }

    vkDestroyRenderPass(app->device, app->render_pass, app->vkalloc);

    vkDestroySwapchainKHR(app->device, app->swapchain.handle, app->vkalloc);
}

static void
appSetupSwapchain(App *app)
{
    if (app->swapchain.handle)
    {
        VK_CHECK(vkDeviceWaitIdle(app->device));
        appCleanupSwapchain(app);
    }

    appCreateSwapchain(app);
    appCreateRenderPass(app);
    appCreatePipeline(app);
    appCreateFrameBuffers(app);
}

static U32
appFindMemoryType(App *app, VkMemoryPropertyFlags type_flags, U32 type_filter)
{
    VkPhysicalDeviceMemoryProperties mem_properties;
    vkGetPhysicalDeviceMemoryProperties(app->gpu, &mem_properties);

    for (U32 index = 0; index < mem_properties.memoryTypeCount; ++index)
    {
        VkMemoryType *type = mem_properties.memoryTypes + index;

        if ((type_filter & (1 << index)) && (type->propertyFlags & type_flags) == type_flags)
        {
            return index;
        }
    }

    return U32_MAX;
}

static void
appCreateBuffer(App *app, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags flags,
                bool mapped, ResourceBuffer *buffer)
{
    // Create buffer
    VkBufferCreateInfo buffer_info = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = size, // NOTE (Matteo): Size in bytes!
        .usage = usage,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
    };
    VK_CHECK(vkCreateBuffer(app->device, &buffer_info, app->vkalloc, &buffer->buffer));

    // Allocate memory for the buffer
    VkMemoryRequirements requirements;
    vkGetBufferMemoryRequirements(app->device, buffer->buffer, &requirements);

    VkMemoryAllocateInfo alloc_info = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = requirements.size,
        .memoryTypeIndex = appFindMemoryType(app, flags, requirements.memoryTypeBits),
    };

    if (alloc_info.memoryTypeIndex == U32_MAX)
    {
        appTerminate(app, "Cannot find a suitable memory type for the buffer");
    }

    VK_CHECK(vkAllocateMemory(app->device, &alloc_info, app->vkalloc, &buffer->memory));

    // Bind memory to the buffer
    VK_CHECK(vkBindBufferMemory(app->device, buffer->buffer, buffer->memory, 0));

    buffer->size = size;
    buffer->map = NULL;

    if (mapped)
    {
        VK_CHECK(vkMapMemory(app->device, buffer->memory, 0, buffer->size, 0, &buffer->map));
    }
}

static void
appDestroyBuffer(App *app, ResourceBuffer *buffer)
{
    if (buffer->map)
    {
        vkUnmapMemory(app->device, buffer->memory);
    }

    vkDestroyBuffer(app->device, buffer->buffer, app->vkalloc);
    vkFreeMemory(app->device, buffer->memory, app->vkalloc);
}

static void
appCopyBuffer(App *app, VkBuffer src, VkBuffer dst, VkDeviceSize size)
{
    VkCommandPool cmd_pool = app->frames[0].cmd_pool; // TODO (Matteo): Dedicated pool?

    VkCommandBufferAllocateInfo alloc_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandPool = cmd_pool,
        .commandBufferCount = 1,
    };

    VkCommandBuffer cmd;
    VK_CHECK(vkAllocateCommandBuffers(app->device, &alloc_info, &cmd));

    VkCommandBufferBeginInfo begin_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
    };

    VK_CHECK(vkBeginCommandBuffer(cmd, &begin_info));
    vkCmdCopyBuffer(cmd, src, dst, 1, &(VkBufferCopy){.size = size});
    VK_CHECK(vkEndCommandBuffer(cmd));

    VkSubmitInfo submitInfo = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .commandBufferCount = 1,
        .pCommandBuffers = &cmd,
    };

    VK_CHECK(vkQueueSubmit(app->queue[GRAPHICS], 1, &submitInfo, VK_NULL_HANDLE));
    VK_CHECK(vkQueueWaitIdle(app->queue[GRAPHICS]));

    vkFreeCommandBuffers(app->device, cmd_pool, 1, &cmd);
}

static void
appCreateAndFillBuffer(App *app, void const *data, Usize data_size, VkBufferUsageFlags usage,
                       ResourceBuffer *buffer)
{
    // VK_MEMORY_PROPERTY_HOST_COHERENT_BIT ensures that the mapped memory always matches the
    // contents of the allocated memory. Do keep in mind that this may lead to slightly worse
    // performance than explicit flushing, but we'll see why that doesn't matter in the next
    // chapter.

    // Create an host visible staging buffer
    ResourceBuffer staging = {0};
    appCreateBuffer(app, data_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                    true, &staging);

    // Copy data into the staging buffer
    memCopy(data, staging.map, staging.size);

    // Create a device local vertex buffer
    appCreateBuffer(app, staging.size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | usage,
                    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, false, buffer);

    appCopyBuffer(app, staging.buffer, buffer->buffer, buffer->size);

    appDestroyBuffer(app, &staging);
}

static void
framebufferResizeCallback(GLFWwindow *window, int width, int height)
{
    CF_UNUSED(width);
    CF_UNUSED(height);
    App *app = glfwGetWindowUserPointer(window);
    app->rebuild_swapchain = true;
}

static void
appInitGui(App *app, Platform *platform)
{
    // NOTE (Matteo): Custom IMGUI ini file
// TODO (Matteo): Clean up!
#if CF_COMPILER_MSVC
#    pragma warning(push)
#    pragma warning(disable : 4221)
#endif
    Paths *paths = platform->paths;
    Char8 gui_ini[Paths_Size] = {0};
    CF_ASSERT(paths->base.len + paths->exe_name.len < Paths_Size, "IMGUI ini file name too long");
    memCopy(paths->base.buf, gui_ini, paths->base.len);
    memCopy(paths->exe_name.buf, gui_ini + paths->base.len, paths->exe_name.len);
    pathChangeExt(strFromCstr(gui_ini), strLiteral(".gui"), gui_ini);

    // Setup Dear ImGui context
    platform->gui = &(Gui){
        .alloc = platform->heap,
        .ini_filename = gui_ini,
        .user_data = app,
    };
    guiInit(platform->gui);
#if CF_COMPILER_MSVC
#    pragma warning(pop)
#endif

    // Setup DPI handling
    F32 win_x_scale, win_y_scale;
    glfwGetWindowContentScale(app->window, &win_x_scale, &win_y_scale);
    // HACK How do I get the platform base DPI?
    F32 dpi_scale = win_x_scale > win_y_scale ? win_y_scale : win_x_scale;

    // Setup Dear ImGui style
    guiSetupStyle(GuiTheme_Dark, dpi_scale);

    // Setup Dear ImGui fonts
    ImFontAtlas *fonts = guiFonts();
    if (!guiLoadCustomFonts(fonts, dpi_scale, paths->data)) guiLoadDefaultFont(fonts);

    ImGui_ImplGlfw_InitForVulkan(app->window, true);
}

static void
appCreateFrames(App *app)
{
    for (Usize index = 0; index < CF_ARRAY_SIZE(app->frames); ++index)
    {
        Frame *frame = app->frames + index;

        // Create command pool and buffers

        VkCommandPoolCreateInfo pool_info = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
            .queueFamilyIndex = app->queue_index[GRAPHICS],
        };
        VK_CHECK(vkCreateCommandPool(app->device, &pool_info, app->vkalloc, &frame->cmd_pool));

        VkCommandBufferAllocateInfo cmd_info = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .commandPool = frame->cmd_pool,
            .commandBufferCount = 1,
        };
        VK_CHECK(vkAllocateCommandBuffers(app->device, &cmd_info, &frame->cmd));

        // Create synchronization primitives

        VkSemaphoreCreateInfo sema_info = {.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
        VkFenceCreateInfo fence_info = {
            .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
            .flags = VK_FENCE_CREATE_SIGNALED_BIT,
        };

        VK_CHECK(vkCreateSemaphore(app->device, &sema_info, app->vkalloc, &frame->image_available));
        VK_CHECK(vkCreateSemaphore(app->device, &sema_info, app->vkalloc, &frame->render_finished));
        VK_CHECK(vkCreateFence(app->device, &fence_info, app->vkalloc, &frame->fence));

        // Create uniform buffer

        appCreateBuffer(app, sizeof(UniformBufferObject), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                        true, &frame->uniform);

        // Create a descriptor for the uniform buffer

        VkDescriptorSetAllocateInfo desc_info = {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
            .descriptorPool = app->desc_pool,
            .descriptorSetCount = 1,
            .pSetLayouts = &app->desc_layout,
        };

        VK_CHECK(vkAllocateDescriptorSets(app->device, &desc_info, &frame->desc));

        VkWriteDescriptorSet desc_write = {
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet = frame->desc,
            .dstBinding = 0,
            .dstArrayElement = 0,
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .descriptorCount = 1,
            .pBufferInfo =
                &(VkDescriptorBufferInfo){
                    .buffer = frame->uniform.buffer,
                    .offset = 0,
                    .range = VK_WHOLE_SIZE, // Equivalent to sizeof(UniformBufferObject)
                },
        };

        vkUpdateDescriptorSets(app->device, 1, &desc_write, 0, NULL);
    }
}

static void
appDestroyFrames(App *app)
{
    for (U32 index = 0; index < CF_ARRAY_SIZE(app->frames); ++index)
    {
        Frame *frame = app->frames + index;
        appDestroyBuffer(app, &frame->uniform);
        vkDestroySemaphore(app->device, frame->image_available, app->vkalloc);
        vkDestroySemaphore(app->device, frame->render_finished, app->vkalloc);
        vkDestroyFence(app->device, frame->fence, app->vkalloc);
        vkDestroyCommandPool(app->device, frame->cmd_pool, app->vkalloc);
    }
}

void
appInit(App *app, Platform *platform)
{
    app->platform = platform;
    app->allocator = platform->heap;
    app->vkalloc = NULL; // TODO (Matteo): Implement

    //=== Initialize GLFW and create window  ===//

    glfwInit();
    // NOTE (Matteo): Don't require OpenGL
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    app->window = glfwCreateWindow(800, 600, "Vulkan window", NULL, NULL);

    glfwSetWindowUserPointer(app->window, app);
    glfwSetFramebufferSizeCallback(app->window, framebufferResizeCallback);

    appInitGui(app, platform);

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
    inst_info.ppEnabledLayerNames = g_layers;
    inst_info.enabledLayerCount = CF_ARRAY_SIZE(g_layers);
    // Add debug extension
    extensions[num_extensions++] = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;
#endif

    inst_info.enabledExtensionCount = num_extensions;
    inst_info.ppEnabledExtensionNames = extensions;

    VK_CHECK(vkCreateInstance(&inst_info, app->vkalloc, &app->inst));

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
    VK_CHECK(vkCreateDebugUtilsMessengerEXT(app->inst, &debug_info, app->vkalloc, &app->debug));

    // Retrieve a Vulkan surface from GLFW window
    VK_CHECK(glfwCreateWindowSurface(app->inst, app->window, app->vkalloc, &app->surface));

    // Choose a suitable GPU for rendering
    appPickGpu(app);
    appPrintGpuMemoryProperties(app);

    // Create a logical device with associated graphics and presentation queues
    appCreateLogicalDevice(app);

    // Create the descriptor pool
    VkDescriptorPoolSize pool_sizes[] = {
        {.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, .descriptorCount = 1},
        {.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, .descriptorCount = CF_ARRAY_SIZE(app->frames)},
    };

    VkDescriptorPoolCreateInfo pool_info = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
        .poolSizeCount = CF_ARRAY_SIZE(pool_sizes),
        .pPoolSizes = pool_sizes,
    };

    for (Usize index = 0; index < CF_ARRAY_SIZE(pool_sizes); ++index)
    {
        pool_info.maxSets += pool_sizes[index].descriptorCount;
    }

    VK_CHECK(vkCreateDescriptorPool(app->device, &pool_info, app->vkalloc, &app->desc_pool));

    appCreateDescriptorSetLayout(app);

    // Create frames
    appCreateFrames(app);

    appCreateAndFillBuffer(app, g_vertices, sizeof(g_vertices), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                           &app->vertex);

    appCreateAndFillBuffer(app, g_indices, sizeof(g_indices), VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                           &app->index);
}

void
appShutdown(App *app)
{
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    guiShutdown(app->platform->gui);

    appDestroyBuffer(app, &app->index);
    appDestroyBuffer(app, &app->vertex);

    appDestroyFrames(app);
    appCleanupSwapchain(app);

    vkDestroyDescriptorSetLayout(app->device, app->desc_layout, app->vkalloc);
    vkDestroyDescriptorPool(app->device, app->desc_pool, app->vkalloc);

    vkDestroyDevice(app->device, app->vkalloc);
    vkDestroySurfaceKHR(app->inst, app->surface, app->vkalloc);
    vkDestroyDebugUtilsMessengerEXT(app->inst, app->debug, app->vkalloc);
    vkDestroyInstance(app->inst, app->vkalloc);

    glfwDestroyWindow(app->window);
    glfwTerminate();
}

//---------------//
//   App logic   //
//---------------//

static void
appSetupGuiRendering(App *app)
{
    if (app->gui_setup) return;

    // Initialize the backend
    {
        ImGui_ImplVulkan_InitInfo imgui_info = {
            .Allocator = app->vkalloc,
            .Instance = app->inst,
            .Device = app->device,
            .PhysicalDevice = app->gpu,
            .Queue = app->queue[GRAPHICS],
            .QueueFamily = app->queue_index[GRAPHICS],
            .Subpass = 0,
            .MinImageCount = app->swapchain.image_count,
            .ImageCount = app->swapchain.image_count,
            .DescriptorPool = app->desc_pool,
            .MSAASamples = VK_SAMPLE_COUNT_1_BIT,
            .CheckVkResultFn = guiCheckResult,
        };

        ImGui_ImplVulkan_Init(&imgui_info, app->render_pass);
    }

    // Upload Fonts
    {
        // Use any command queue
        VkCommandPool command_pool = app->frames[0].cmd_pool;
        VkCommandBuffer command_buffer = app->frames[0].cmd;

        VK_CHECK(vkResetCommandPool(app->device, command_pool, 0));

        VkCommandBufferBeginInfo begin_info = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
        };
        VK_CHECK(vkBeginCommandBuffer(command_buffer, &begin_info));

        ImGui_ImplVulkan_CreateFontsTexture(command_buffer);

        VkSubmitInfo end_info = {
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .commandBufferCount = 1,
            .pCommandBuffers = &command_buffer,
        };
        VK_CHECK(vkEndCommandBuffer(command_buffer));
        VK_CHECK(vkQueueSubmit(app->queue[GRAPHICS], 1, &end_info, VK_NULL_HANDLE));

        VK_CHECK(vkDeviceWaitIdle(app->device));
        ImGui_ImplVulkan_DestroyFontUploadObjects();
    }

    app->gui_setup = true;
}

static void
appDrawFrame(App *app, Frame *frame)
{
    Swapchain *swapchain = &app->swapchain;

    // Synchronize current frame with the graphics queue
    vkWaitForFences(app->device, 1, &frame->fence, VK_TRUE, U64_MAX);

    // Update the UBO on then current frame
    CF_ASSERT(frame->uniform.size >= sizeof(app->ubo), "Data error");
    memCopy(&app->ubo, frame->uniform.map, sizeof(app->ubo));

    // Request a backbuffer from the swapchain
    U32 image_index = 0;
    VkResult res = vkAcquireNextImageKHR(app->device, swapchain->handle, U64_MAX,
                                         frame->image_available, VK_NULL_HANDLE, &image_index);

    if (res == VK_ERROR_OUT_OF_DATE_KHR)
    {
        app->rebuild_swapchain = true;
        return;
    }
    else if (res != VK_SUBOPTIMAL_KHR)
    {
        VK_CHECK(res);
    }

    // Record the commands
    // NOTE (Matteo): The commands here are static and so could be pre-recorded, but the idea is
    // to build a more realistic setup
    {
        VkClearValue clear_color = {.color = {{0.0f, 0.0f, 0.0f, 1.0f}}};

        VkCommandBufferBeginInfo cmd_begin_info = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        };

        VkRenderPassBeginInfo pass_begin_info = {
            .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
            .renderPass = app->render_pass,
            .renderArea = {.offset = {0, 0}, .extent = swapchain->extent},
            .pClearValues = &clear_color,
            .clearValueCount = 1,
        };

        VK_CHECK(vkResetCommandPool(app->device, frame->cmd_pool, 0));
        VK_CHECK(vkBeginCommandBuffer(frame->cmd, &cmd_begin_info));

        pass_begin_info.framebuffer = swapchain->frame[image_index];
        vkCmdBeginRenderPass(frame->cmd, &pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);
        vkCmdBindPipeline(frame->cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, app->pipe);

        VkDeviceSize offset = 0;
        vkCmdBindVertexBuffers(frame->cmd, 0, 1, &app->vertex.buffer, &offset);
        vkCmdBindIndexBuffer(frame->cmd, app->index.buffer, 0, VK_INDEX_TYPE_UINT16);

        vkCmdBindDescriptorSets(frame->cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, app->pipe_layout, 0, 1,
                                &frame->desc, 0, NULL);

        // vkCmdDraw(frame->cmd,
        //           CF_ARRAY_SIZE(g_vertices), // Number of vertices
        //           1, // Number of instances - Use 1 if not using instanced rendering
        //           0, // First vertex - lowest value for 'gl_VertexIndex'
        //           0  // First instance - lowest value for 'gl_InstanceIndex'
        // );

        vkCmdDrawIndexed(frame->cmd,
                         CF_ARRAY_SIZE(g_indices), // Number of indices
                         1, // Number of instances - Use 1 if not using instanced rendering
                         0, // First index - lowest value for 'gl_VertexIndex'
                         0, // Offset to be added to the indices
                         0  // First instance - lowest value for 'gl_InstanceIndex'
        );

        ImDrawData *draw_data = guiRender();
#if RENDER_GUI
        ImGui_ImplVulkan_RenderDrawData(draw_data, frame->cmd, VK_NULL_HANDLE);
#else
        CF_UNUSED(draw_data);
#endif

        vkCmdEndRenderPass(frame->cmd);
        VK_CHECK(vkEndCommandBuffer(frame->cmd));
    }

    // Submit the commands to the queue
    {
        VkPipelineStageFlags wait_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

        VkSubmitInfo submit_info = {
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .commandBufferCount = 1,
            .pCommandBuffers = &frame->cmd,
            .waitSemaphoreCount = 1,
            .pWaitSemaphores = &frame->image_available,
            .pWaitDstStageMask = &wait_stage,
            .signalSemaphoreCount = 1,
            .pSignalSemaphores = &frame->render_finished,
        };

        VK_CHECK(vkResetFences(app->device, 1, &frame->fence));
        VK_CHECK(vkQueueSubmit(app->queue[GRAPHICS], 1, &submit_info, frame->fence));
    }

    // Present the rendered image
    {
        VkPresentInfoKHR present_info = {
            .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
            .waitSemaphoreCount = 1,
            .pWaitSemaphores = &frame->render_finished,
            .swapchainCount = 1,
            .pSwapchains = &swapchain->handle,
            .pImageIndices = &image_index,
        };

        res = vkQueuePresentKHR(app->queue[PRESENT], &present_info);
        if (res == VK_ERROR_OUT_OF_DATE_KHR || res == VK_SUBOPTIMAL_KHR)
        {
            app->rebuild_swapchain = true;
        }
        else
        {
            VK_CHECK(res);
        }
    }
}

void
appAnimate(App *app)
{
    UniformBufferObject *ubo = &app->ubo;

    Duration elapsed = clockElapsed(&app->clock);
    F32 seconds = (F32)timeGetSeconds(elapsed);

    Mat4 rot = matRotation(VEC3_Z, seconds * mRadians(90.0f));

    Vec3 eye = {.x = 0, .y = 2, .z = 2};
    // eye = matMul(rot, eye);
    // eye.z += (1 + mSin(seconds));

    ubo->model = rot;
    ubo->view = matLookAt(eye,                        // Eye
                          (Vec3){{0.0f, 0.0f, 0.0f}}, // Center
                          (Vec3){{0.0f, 1.0f, 0.0f}}  // Up Axis
    );
}

void
appMainLoop(App *app)
{
    // NOTE (Matteo): Ensure swapchain setup on first loop
    app->rebuild_swapchain = (app->swapchain.handle == VK_NULL_HANDLE);

    // Setup static transforms
    UniformBufferObject *ubo = &app->ubo;
    ubo->model = matIdentity();
    ubo->proj = matIdentity();
    // Look at the geometry from above at a 45 degree angle.
    ubo->view = matLookAt((Vec3){{0, -3, 2}},         // Eye
                          (Vec3){{0.0f, 0.0f, 0.0f}}, // Center
                          (Vec3){{0.0f, 1.0f, 0.0f}}  // Up Axis
    );
    // Vulkan clip space with reverse-depth
    // ClipSpace clip = mClipSpaceVk(true);
    ClipSpace clip = {.y_dir = -1, .z_near = -1, .z_far = 1};

    // Start time tracking
    clockStart(&app->clock);

    while (!glfwWindowShouldClose(app->window))
    {
        glfwPollEvents();

        if (app->rebuild_swapchain)
        {
            // NOTE (Matteo): Minimization can render the swapchain out of date, but
            // it is useless to setup one, so we just sit here waiting.
            while (glfwGetWindowAttrib(app->window, GLFW_ICONIFIED))
            {
                glfwWaitEvents();
            }

            appSetupSwapchain(app);
            app->rebuild_swapchain = false;

            // Perspective projection with a 45 degree vertical field-of-view.
            F32 width = (F32)app->swapchain.extent.width;
            F32 height = (F32)app->swapchain.extent.height;
            ubo->proj = matPerspective(mRadians(45.0f), (width / height), 0.1f, 10.0f, clip);
        }

        appSetupGuiRendering(app);
        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        guiNewFrame();
        guiDemoWindow(NULL);

        appAnimate(app);

        static_assert(cfIsPowerOf2(CF_ARRAY_SIZE(app->frames)), "Frame count is not a power of 2!");

        Usize frame_mask = CF_ARRAY_SIZE(app->frames) - 1;
        Frame *frame = app->frames + (app->frame_index & frame_mask);
        appDrawFrame(app, frame);
        app->frame_index++;

        // Update and Render additional Platform Windows
        if (guiViewportsEnabled()) guiUpdateViewports(RENDER_GUI);
    }

    VK_CHECK(vkDeviceWaitIdle(app->device));
}
