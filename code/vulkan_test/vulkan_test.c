#include "platform.h"

// Backend libraries
#define GLFW_INCLUDE_NONE
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

typedef struct Swapchain
{
    VkSwapchainKHR handle;
    VkExtent2D extent;
    VkImageView image[3];
    VkFramebuffer frame[3];
    VkFence fence[3];
    U32 image_count;
    VkFormat format;
} Swapchain;

typedef struct Frame
{
    VkCommandPool cmd_pool;
    VkCommandBuffer cmd;

    VkFence fence;

    VkSemaphore image_available;
    VkSemaphore render_finished;
} Frame;

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
    VkQueue queue[2];
    U32 queue_index[2];

    Swapchain swapchain;

    VkRenderPass render_pass;
    VkPipelineLayout pipe_layout;
    VkPipeline pipe;

    Frame frames[2];
    Usize frame_index;
} App;

static void appInit(App *app, Platform *platform);
static void appShutdown(App *app);
static void appMainLoop(App *app);

//-------------//
//   Globals   //
//-------------//

// Fragment sharder bytecode
static U32 const g_frag_code[] = {
#include "frag.spv"
};

// Vertex sharder bytecode
static U32 const g_vert_code[] = {
#include "vert.spv"
};

// Vulkan debug layers
static Cstr const g_layers[] = {
    "VK_LAYER_KHRONOS_validation",
};

// Configure the pipeline state that can change dynamically
static VkDynamicState const g_dynamic_states[] = {
    // VK_DYNAMIC_STATE_VIEWPORT,
    VK_DYNAMIC_STATE_LINE_WIDTH,
};

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
    CF_UNUSED(cmd_line);

    App app = {0};
    appInit(&app, platform);
    appMainLoop(&app);
    appShutdown(&app);

    return 0;
}

//------------------------//
//   Vulkan debug layer   //
//------------------------//

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
        .ppEnabledLayerNames = g_layers,
        .enabledLayerCount = CF_ARRAY_SIZE(g_layers),
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
    Swapchain *swapchain = &app->swapchain;

    VkSurfaceCapabilitiesKHR caps;
    VkResult res = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(app->gpu, app->surface, &caps);
    appCheckResult(app, res);

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

        swapchain->format = info.imageFormat;
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

    // NOTE (Matteo): Request one image more than the minimum to avoid excessive waiting on the
    // driver
    info.minImageCount = caps.minImageCount + 1;
    if (caps.maxImageCount > 0 && info.minImageCount > caps.maxImageCount)
    {
        info.minImageCount = caps.maxImageCount;
    }

    res = vkCreateSwapchainKHR(app->device, &info, app->vkalloc, &swapchain->handle);
    appCheckResult(app, res);
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

    VkResult res = vkCreateRenderPass(app->device, &info, app->vkalloc, &app->render_pass);
    appCheckResult(app, res);
}

static VkShaderModule
appCreateShaderModule(App *app, U32 const *code, Usize code_size)
{
    VkShaderModule module;
    VkResult res = vkCreateShaderModule(app->device,
                                        &(VkShaderModuleCreateInfo){
                                            .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
                                            .pCode = code,
                                            .codeSize = code_size,
                                        },
                                        app->vkalloc, &module);
    appCheckResult(app, res);
    return module;
}

static void
appCreatePipeline(App *app)
{
    VkPipelineShaderStageCreateInfo stage_info[2] = {
        {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .module = appCreateShaderModule(app, g_vert_code, sizeof(g_vert_code)),
            .pName = "main",
            .stage = VK_SHADER_STAGE_VERTEX_BIT,
        },
        {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .module = appCreateShaderModule(app, g_frag_code, sizeof(g_frag_code)),
            .pName = "main",
            .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
        },
    };

    VkPipelineVertexInputStateCreateInfo input_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
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
        .cullMode = VK_CULL_MODE_BACK_BIT,
        .frontFace = VK_FRONT_FACE_CLOCKWISE,
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
#if 1
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
        .dynamicStateCount = CF_ARRAY_SIZE(g_dynamic_states),
        .pDynamicStates = g_dynamic_states,
    };

    // TODO (Matteo): Revisit - Creating an empty layout for now
    VkPipelineLayoutCreateInfo layout_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount = 0,
    };

    VkResult res =
        vkCreatePipelineLayout(app->device, &layout_info, app->vkalloc, &app->pipe_layout);
    appCheckResult(app, res);

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

    res = vkCreateGraphicsPipelines(app->device, VK_NULL_HANDLE, 1, &pipe_info, app->vkalloc,
                                    &app->pipe);
    appCheckResult(app, res);

    vkDestroyShaderModule(app->device, stage_info[0].module, app->vkalloc);
    vkDestroyShaderModule(app->device, stage_info[1].module, app->vkalloc);
}

static void
appCreateFrameBuffers(App *app)
{
    Swapchain *swapchain = &app->swapchain;

    VkResult res =
        vkGetSwapchainImagesKHR(app->device, swapchain->handle, &swapchain->image_count, NULL);
    appCheckResult(app, res);

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
        res = vkCreateImageView(app->device, &image_info, app->vkalloc, swapchain->image + index);
        appCheckResult(app, res);

        frame_info.pAttachments = swapchain->image + index;
        res = vkCreateFramebuffer(app->device, &frame_info, app->vkalloc, swapchain->frame + index);
        appCheckResult(app, res);
    }
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
    inst_info.ppEnabledLayerNames = g_layers;
    inst_info.enabledLayerCount = CF_ARRAY_SIZE(g_layers);
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

    // Setup rendering swapchain and pipeline
    appCreateSwapchain(app);
    appCreateRenderPass(app);
    appCreatePipeline(app);
    appCreateFrameBuffers(app);

    // Create frames
    for (Usize index = 0; index < CF_ARRAY_SIZE(app->frames); ++index)
    {
        Frame *frame = app->frames + index;

        VkCommandPoolCreateInfo pool_info = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
            .queueFamilyIndex = app->queue_index[GRAPHICS],
        };
        res = vkCreateCommandPool(app->device, &pool_info, app->vkalloc, &frame->cmd_pool);
        appCheckResult(app, res);

        VkCommandBufferAllocateInfo cmd_info = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .commandPool = frame->cmd_pool,
            .commandBufferCount = 1,
        };
        res = vkAllocateCommandBuffers(app->device, &cmd_info, &frame->cmd);

        VkSemaphoreCreateInfo sema_info = {.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
        VkFenceCreateInfo fence_info = {
            .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
            .flags = VK_FENCE_CREATE_SIGNALED_BIT,
        };

        res = vkCreateSemaphore(app->device, &sema_info, app->vkalloc, &frame->image_available);
        appCheckResult(app, res);

        res = vkCreateSemaphore(app->device, &sema_info, app->vkalloc, &frame->render_finished);
        appCheckResult(app, res);

        res = vkCreateFence(app->device, &fence_info, app->vkalloc, &frame->fence);
        appCheckResult(app, res);
    }
}

void
appShutdown(App *app)
{
    for (U32 index = 0; index < CF_ARRAY_SIZE(app->frames); ++index)
    {
        Frame *frame = app->frames + index;
        vkDestroySemaphore(app->device, frame->image_available, app->vkalloc);
        vkDestroySemaphore(app->device, frame->render_finished, app->vkalloc);
        vkDestroyFence(app->device, frame->fence, app->vkalloc);
        vkDestroyCommandPool(app->device, frame->cmd_pool, app->vkalloc);
    }

    vkDestroyPipeline(app->device, app->pipe, app->vkalloc);
    vkDestroyPipelineLayout(app->device, app->pipe_layout, app->vkalloc);

    for (U32 index = 0; index < app->swapchain.image_count; ++index)
    {
        vkDestroyFramebuffer(app->device, app->swapchain.frame[index], app->vkalloc);
        vkDestroyImageView(app->device, app->swapchain.image[index], app->vkalloc);
    }

    vkDestroyRenderPass(app->device, app->render_pass, app->vkalloc);

    vkDestroySwapchainKHR(app->device, app->swapchain.handle, app->vkalloc);
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
appDrawFrame(App *app, Frame *frame)
{
    VkResult res;
    Swapchain *swapchain = &app->swapchain;

    // Synchronize current frame with the graphics queue
    vkWaitForFences(app->device, 1, &frame->fence, VK_TRUE, U64_MAX);

    // Request a backbuffer from the swapchain
    U32 image_index = 0;
    res = vkAcquireNextImageKHR(app->device, swapchain->handle, U64_MAX, frame->image_available,
                                VK_NULL_HANDLE, &image_index);
    appCheckResult(app, res);

    // Record the commands
    // NOTE (Matteo): The commands here are static and so could be pre-recorded, but the idea is to
    // build a more realistic setup
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

        res = vkResetCommandPool(app->device, frame->cmd_pool, 0);
        appCheckResult(app, res);

        res = vkBeginCommandBuffer(frame->cmd, &cmd_begin_info);
        appCheckResult(app, res);

        pass_begin_info.framebuffer = swapchain->frame[image_index];
        vkCmdBeginRenderPass(frame->cmd, &pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);
        vkCmdBindPipeline(frame->cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, app->pipe);

        vkCmdDraw(frame->cmd,
                  3, // Vertex number
                  1, // Instance number - Use 1 if not using instanced rendering
                  0, // First vertex - lowest value for 'gl_VertexIndex'
                  0  // First instance - lowest value for 'gl_InstanceIndex'
        );

        vkCmdEndRenderPass(frame->cmd);
        res = vkEndCommandBuffer(frame->cmd);
        appCheckResult(app, res);
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

        res = vkResetFences(app->device, 1, &frame->fence);
        appCheckResult(app, res);

        res = vkQueueSubmit(app->queue[GRAPHICS], 1, &submit_info, frame->fence);
        appCheckResult(app, res);
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
        appCheckResult(app, res);
    }
}

void
appMainLoop(App *app)
{
    while (!glfwWindowShouldClose(app->window))
    {
        glfwPollEvents();

        Frame *frame = app->frames + (app->frame_index & 1);
        appDrawFrame(app, frame);
        app->frame_index++;
    }

    vkDeviceWaitIdle(app->device);
}
