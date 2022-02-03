#pragma once

#include "foundation/core.h"

typedef struct ImGuiContext GuiContext;
typedef struct ImFontAtlas GuiFontAtlas;

//=== Initialization ===//

typedef struct GuiOpenGLVersion
{
    U32 major, minor;
    /// Shader version for Dear Imgui OpenGL backend
    U32 glsl;
} GuiOpenGLVersion;

/// IMGUI state, used to initialize internal global variables
typedef struct GuiInitInfo
{
    MemAllocator alloc;
    GuiFontAtlas *shared_atlas;
    Cstr ini_filename;
    void *user_data;
    Str data_path;
    bool gl_context;
} GuiInitInfo;

/// Initialize IMGUI global state
CF_API GuiContext *guiInit(GuiInitInfo *info, GuiOpenGLVersion *out_gl_version);
CF_API void guiShutdown(GuiContext *ctx);

//=== Updating ===//

CF_API void guiSetTitle(Cstr title);
CF_API bool guiEventLoop(bool blocking, bool fullscreen, IVec2 *display);
CF_API void guiUpdateViewports(bool render);

//=== Vulkan support ===//

// Avoid including <vulkan.h> so we can build without it
#ifndef VULKAN_H_
#    define VK_DEFINE_HANDLE(object) typedef struct object##_T *object;
#    if defined(__LP64__) || defined(_WIN64) || defined(__x86_64__) || defined(_M_X64) || \
        defined(__ia64) || defined(_M_IA64) || defined(__aarch64__) || defined(__powerpc64__)
#        define VK_DEFINE_NON_DISPATCHABLE_HANDLE(object) typedef struct object##_T *object;
#    else
#        define VK_DEFINE_NON_DISPATCHABLE_HANDLE(object) typedef uint64_t object;
#    endif

VK_DEFINE_HANDLE(VkInstance)
VK_DEFINE_NON_DISPATCHABLE_HANDLE(VkSurfaceKHR)

typedef struct VkAllocationCallbacks VkAllocationCallbacks;

#    undef VK_DEFINE_HANDLE
#    undef VK_DEFINE_NON_DISPATCHABLE_HANDLE
#endif // VULKAN_H_

CF_API Cstr const *guiRequiredVulkanExtensions(U32 *count);
CF_API I32 guiCreateVulkanSurface(VkInstance vk_instance, const VkAllocationCallbacks *vk_allocator,
                                  VkSurfaceKHR *out_vk_surface);
