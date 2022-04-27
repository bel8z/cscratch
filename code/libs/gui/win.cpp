#include "win.h"

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include "gui_backend_glfw.cpp"
#include "gui_internal.h"

#include "foundation/memory.h"

// NOTE (Matteo):
// The way GLFW initializes OpenGL context on windows is a bit peculiar.
// To create a context of the most recent version supported by the driver, one must require
// version 1.0 (which is also the default hint value); in this case the "forward compatibility" and
// "core profile" flags are not supported, and window creation fails.
// On the other side, setting a specific 3.0+ version is not merely an hint, because a context of
// that same version is created (or window creation fails if not supported); not sure if this is a
// GLFW or WGL bug (or feature? D: ).
// Anyway, I work around this by trying some different versions in decreasing release
// order (one per year or release), in order to benefit of the latest features available.

CF_GLOBAL const GuiOpenGLVersion g_gl_versions[8] = {
    {4, 6, 430}, // 2017
    {4, 5, 430}, // 2014
    {4, 4, 430}, // 2013
    {4, 3, 430}, // 2012
    {4, 2, 420}, // 2011
    {4, 1, 410}, // 2010
    {3, 2, 150}, // 2009
    {3, 0, 130}, // 2008
};

CF_INTERNAL GLFWwindow *
gui_CreateWinAndContext(Cstr title, I32 width, I32 height, GuiOpenGLVersion *gl_version)
{
    GLFWwindow *window = NULL;

    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_API);
    glfwWindowHint(GLFW_SRGB_CAPABLE, GLFW_TRUE);
    glfwWindowHint(GLFW_DOUBLEBUFFER, GLFW_TRUE);

    for (Usize i = 0; i < CF_ARRAY_SIZE(g_gl_versions); ++i)
    {
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, (I32)g_gl_versions[i].major);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, (I32)g_gl_versions[i].minor);

        if (g_gl_versions[i].major >= 3)
        {
            // TODO (Matteo): Make this OS specific? It doesn't seem to bring any penalty
            // 3.0+ only, required on MAC
            glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

            if (g_gl_versions[i].minor >= 2)
            {
                // 3.2+ only
                glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
            }
        }

        window = glfwCreateWindow(width, height, title, NULL, NULL);
        if (window)
        {
            *gl_version = g_gl_versions[i];
            break;
        }
    }

    return window;
}

CF_INTERNAL inline GLFWwindow *
gui_MainWindow(void)
{
    return (GLFWwindow *)guiData().main_window;
}

CF_INTERNAL void
gui_ErrorCallback(int error, Cstr description)
{
    CF_UNUSED(error);
    CF_UNUSED(description);
    // logError("Glfw Error %d: %s\n", error, description);
}

GuiContext *
guiInit(GuiInitInfo *gui, GuiOpenGLVersion *out_gl_ver)
{
    CF_ASSERT_NOT_NULL(gui);

    GLFWwindow *window = NULL;
    GuiContext *ctx = NULL;

    // Setup platform
    {
        // TODO (Matteo): proper error handling
        glfwSetErrorCallback(gui_ErrorCallback);
        if (!glfwInit()) return NULL;

        // Create window with graphics context
        if (gui->gl_context)
        {
            GuiOpenGLVersion gl_ver{};
            if (!out_gl_ver) out_gl_ver = &gl_ver;
            window = gui_CreateWinAndContext("Dear ImGui template", 1280, 720, out_gl_ver);
        }
        else
        {
            glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
            window = glfwCreateWindow(1280, 720, "Dear ImGui template", NULL, NULL);
        }

        if (window == NULL) return NULL;

        if (gui->gl_context)
        {
            glfwMakeContextCurrent(window);
            glfwSwapInterval(1); // Enable vsync
        }
    }

    // Setup IMGUI
    {
        IMGUI_CHECKVERSION();

        // Configure custom user data
        GuiData *gui_data = (GuiData *)memAlloc(gui->alloc, sizeof(*gui_data));
        gui_data->allocator = gui->alloc;
        gui_data->user_data = gui->user_data;
        gui_data->main_window = window;

        ImGui::SetAllocatorFunctions(guiAlloc, guiFree, gui_data);

        ctx = ImGui::CreateContext(gui->shared_atlas);

        ImGuiIO &io = ImGui::GetIO();

        io.UserData = gui_data;

        io.IniFilename = gui->ini_filename;

        // Enable Keyboard Controls
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        // Enable Docking
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
        // Reduce visual noise while docking, also has a benefit for out-of-sync viewport rendering
        io.ConfigDockingTransparentPayload = true;

#if GUI_VIEWPORTS
        // Enable Multi-Viewport / Platform Windows
        io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
#endif

        guiSetTheme(GuiTheme_EmeraldDark);
    }

    // Setup DPI handling
    F32 dpi_scale;
    {
        F32 win_x_scale, win_y_scale;
        glfwGetWindowContentScale(window, &win_x_scale, &win_y_scale);
        // HACK How do I get the platform base DPI?
        dpi_scale = win_x_scale > win_y_scale ? win_y_scale : win_x_scale;
    }

    // Setup Dear ImGui style
    guiSetupStyle(GuiTheme_EmeraldLight, dpi_scale);
    if (!strValid(gui->data_path) || !guiLoadCustomFonts(guiFonts(), dpi_scale, gui->data_path))
    {
        guiLoadDefaultFont(guiFonts());
    }

    guiGlfwInit(window, gui->gl_context);

    return ctx;
}

void
guiShutdown(GuiContext *ctx)
{
    guiGlfwShutdown();

    CF_ASSERT(ctx == ImGui::GetCurrentContext(), "Inconsistent contexts");

    GuiData *gui_data = (GuiData *)ImGui::GetIO().UserData;

    ImGui::DestroyContext(ctx);

    CF_ASSERT(gui_data->memory.size == 0, "Possible GUI memory leak");
    CF_ASSERT(gui_data->memory.blocks == 0, "Possible GUI memory leak");

    glfwDestroyWindow((GLFWwindow *)gui_data->main_window);

    memFree(gui_data->allocator, gui_data, sizeof(*gui_data));

    glfwTerminate();
}

void
guiSetTitle(Cstr title)
{
    glfwSetWindowTitle(gui_MainWindow(), title);
}

Cstr const *
guiRequiredVulkanExtensions(U32 *count)
{
    return glfwGetRequiredInstanceExtensions(count);
}

I32
guiCreateVulkanSurface(VkInstance vk_instance, const VkAllocationCallbacks *vk_allocator,
                       VkSurfaceKHR *out_vk_surface)
{
    return glfwCreateWindowSurface(vk_instance, gui_MainWindow(), vk_allocator, out_vk_surface);
}

bool
guiEventLoop(bool blocking, bool fullscreen, IVec2 *display)
{
    GLFWwindow *window = gui_MainWindow();

    if (glfwWindowShouldClose(window)) return false;

    glfwSwapBuffers(window);

    if (blocking)
    {
        glfwWaitEvents();
    }
    else
    {
        glfwPollEvents();
    }

    IVec2 win_pos, win_size, framebuff;
    glfwGetWindowPos(window, &win_pos.x, &win_pos.y);
    glfwGetWindowSize(window, &win_size.width, &win_size.height);

    GLFWmonitor *monitor = glfwGetWindowMonitor(window);
    bool is_fullscreen = (monitor != NULL);
    if (fullscreen != is_fullscreen)
    {
        if (is_fullscreen)
        {
            // TODO (Matteo): Investigate freeze when leaving fullscreen mode
            glfwSetWindowMonitor(window, NULL,         //
                                 win_pos.x, win_pos.y, //
                                 win_size.width, win_size.height, 0);
        }
        else
        {
            monitor = glfwGetPrimaryMonitor();
            GLFWvidmode const *vidmode = glfwGetVideoMode(monitor);
            glfwSetWindowMonitor(window, monitor, 0, 0, vidmode->width, vidmode->height,
                                 vidmode->refreshRate);
        }
    }

    glfwGetFramebufferSize(window, &framebuff.width, &framebuff.height);
    guiGlfwNewFrame(win_size, framebuff);

    if (display) *display = framebuff;

    return true;
}

void
guiUpdateViewports(bool render)
{
    // Update and Render additional Platform Windows
    // (Platform functions may change the current OpenGL context, so we save/restore it
    // to make it easier to paste this code elsewhere.)
    if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        GLFWwindow *backup_current_context = glfwGetCurrentContext();

        ImGui::UpdatePlatformWindows();
        if (render)
        {
            ImGui::RenderPlatformWindowsDefault(NULL, NULL);
        }

        glfwMakeContextCurrent(backup_current_context);
    }
}
