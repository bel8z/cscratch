// Interface between platform layer and hosted applicatiom
#include "api.h"

// Gui library
#include "gui/gui.h"

// Backend libraries
#include "gl/gload.h"

#if SDL_BACKEND
#    define SDL_MAIN_HANDLED
#    include <SDL.h>
#else
#    include <GLFW/glfw3.h>
#endif

// Foundation library
#include "foundation/common.h"

#include "foundation/color.h"
#include "foundation/path.h"
#include "foundation/strings.h"

// Standard library
#include <stdio.h>

// Constants for DPI handling
#define PLATFORM_DPI 96.0f
#define TRUETYPE_DPI 72.0f

//------------------------------------------------------------------------------
// Local function declarations
//------------------------------------------------------------------------------

static void *guiAlloc(Usize size, void *state);
static void guiFree(void *mem, void *state);
static void guiSetupStyle(F32 dpi);
static void guiSetupFonts(ImFontAtlas *fonts, F32 dpi, char const *data_path);
static void guiUpdateFonts(ImFontAtlas *fonts, FontOptions *font_opts);

//------------------------------------------------------------------------------
// OpenGL3 backend declarations
//------------------------------------------------------------------------------

extern bool ImGui_ImplOpenGL3_Init(const char *glsl_version);
extern void ImGui_ImplOpenGL3_Shutdown();
extern void ImGui_ImplOpenGL3_NewFrame();
extern void ImGui_ImplOpenGL3_RenderDrawData(ImDrawData *draw_data);
extern bool ImGui_ImplOpenGL3_CreateFontsTexture();
extern void ImGui_ImplOpenGL3_DestroyFontsTexture();
extern bool ImGui_ImplOpenGL3_CreateDeviceObjects();
extern void ImGui_ImplOpenGL3_DestroyDeviceObjects();

typedef struct GlVersion
{
    I32 major, minor;
    /// Shader version for Dear Imgui OpenGL backend
    char const *glsl;
} GlVersion;

#if SDL_BACKEND
//------------------------------------------------------------------------------
// SDL2 backend declarations
//------------------------------------------------------------------------------

extern bool ImGui_ImplSDL2_InitForOpenGL(SDL_Window *window, void *sdl_gl_context);
extern void ImGui_ImplSDL2_Shutdown();
extern void ImGui_ImplSDL2_NewFrame(SDL_Window *window);
extern bool ImGui_ImplSDL2_ProcessEvent(const SDL_Event *event);

#else
//------------------------------------------------------------------------------
// GLFW backend declarations
//------------------------------------------------------------------------------

extern bool ImGui_ImplGlfw_InitForOpenGL(GLFWwindow *window, bool install_callbacks);
extern void ImGui_ImplGlfw_Shutdown();
extern void ImGui_ImplGlfw_NewFrame();

static void
glfwErrorCallback(int error, const char *description)
{
    fprintf(stderr, "Glfw Error %d: %s\n", error, description);
}

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

static const GlVersion g_gl_versions[8] = {
    {.major = 4, .minor = 6, .glsl = "#version 150"}, // 2017
    {.major = 4, .minor = 5, .glsl = "#version 150"}, // 2014
    {.major = 4, .minor = 4, .glsl = "#version 150"}, // 2013
    {.major = 4, .minor = 3, .glsl = "#version 150"}, // 2012
    {.major = 4, .minor = 2, .glsl = "#version 150"}, // 2011
    {.major = 4, .minor = 1, .glsl = "#version 150"}, // 2010
    {.major = 3, .minor = 2, .glsl = "#version 150"}, // 2009
    {.major = 3, .minor = 0, .glsl = "#version 130"}, // 2008
};

static GLFWwindow *
glfwCreateWinAndContext(char const *title, I32 width, I32 height, GlVersion *gl_version)
{
    GLFWwindow *window = NULL;

    for (Usize i = 0; i < CF_ARRAY_SIZE(g_gl_versions); ++i)
    {
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, g_gl_versions[i].major);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, g_gl_versions[i].minor);

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

#endif

//------------------------------------------------------------------------------
// Application API
//------------------------------------------------------------------------------

typedef struct AppApi
{
    AppCreateProc create;
    AppDestroyProc destroy;
    AppUpdateProc update;
} AppApi;

static APP_CREATE(appCreateStub);
static APP_DESTROY(appDestroyStub);
static APP_UPDATE(appUpdateStub);

static void appInit(AppApi *app, cfPlatform *platform);

//------------------------------------------------------------------------------
// Main
//------------------------------------------------------------------------------

I32
platformMain(cfPlatform *platform, char const *argv[], I32 argc)
{
    CF_UNUSED(argc);
    CF_UNUSED(argv);

#if SDL_BACKEND
    // Setup SDL
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER) != 0)
    {
        fprintf(stderr, "Failed to initialize SDL: %s\n", SDL_GetError());
        return -1;
    }

    // Decide GL+GLSL versions
#    ifdef __APPLE__
    // GL 3.2 Core + GLSL 150
    GlVersion gl_ver = {.major = 3, .minor = 2, .glsl = "#version 150"};
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS,
                        SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG); // Always required on Mac
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, gl_ver.major);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, gl_ver.minor);
#    else
    // GL 3.0 + GLSL 130
    GlVersion gl_ver = {.major = 3, .minor = 0, .glsl = "#version 130"};
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, gl_ver.major);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, gl_ver.minor);
#    endif

    // Create window with graphics context
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    u32 window_flags =
        (SDL_WindowFlags)(SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
    SDL_Window *window = SDL_CreateWindow("Dear ImGui template", SDL_WINDOWPOS_CENTERED,
                                          SDL_WINDOWPOS_CENTERED, 1280, 720, window_flags);
    SDL_GLContext gl_context = SDL_GL_CreateContext(window);
    SDL_GL_MakeCurrent(window, gl_context);
    SDL_GL_SetSwapInterval(1); // Enable vsync

#else
    // Setup window
    glfwSetErrorCallback(glfwErrorCallback);
    if (!glfwInit()) return 1;

    // Create window with graphics context
    GlVersion gl_ver = {0};
    GLFWwindow *window = glfwCreateWinAndContext("Dear ImGui template", 1280, 720, &gl_ver);
    if (window == NULL) return 1;
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Enable vsync
#endif

    // Initialize OpenGL loader
    if (!gloadInit(NULL) || !gloadIsSupported(gl_ver.major, gl_ver.minor))
    {
        fprintf(stderr, "Failed to initialize OpenGL loader!\n");
        return -2;
    }

    // Setup platform layer
    CF_ASSERT_NOT_NULL(gl);
    platform->gl = gl;

    // Setup Dear ImGui context
    platform->gui = &(Gui){
        .alloc_func = guiAlloc,
        .free_func = guiFree,
        .alloc_state = platform->heap,
    };

    guiInit(platform->gui);

    // Setup application
    AppApi app_api;
    appInit(&app_api, platform);
    AppState *app = app_api.create(platform, argv, argc);

    // Configure Dear ImGui

    ImGuiIO *io = igGetIO();

    // NOTE (Matteo): Custom IMGUI ini file
    char gui_ini[Paths_Size] = {0};
    strPrintf(gui_ini, Paths_Size, "%s%s", platform->paths->base, platform->paths->exe_name);
    pathChangeExt(gui_ini, ".gui", gui_ini);

    io->IniFilename = gui_ini;

    // Enable Keyboard Controls
    io->ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    // Enable Docking
    io->ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    // Enable Multi-Viewport / Platform Windows
    io->ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
    // Reduce visual noise while docking, also has a benefit for out-of-sync viewport rendering
    io->ConfigDockingTransparentPayload = true;

    // Setup DPI handling
    F32 dpi_scale = 1.0f;

#if SDL_BACKEND
    F32 ddpi, hdpi, vdpi;
    if (SDL_GetDisplayDPI(0, &ddpi, &hdpi, &vdpi) != 0)
    {
        fprintf(stderr, "Failed to obtain DPI information for display 0: %s\n", SDL_GetError());
        return -3;
    }
    dpi_scale = ddpi / PLATFORM_DPI;

#else
    F32 win_x_scale, win_y_scale;
    glfwGetWindowContentScale(window, &win_x_scale, &win_y_scale);
    // HACK How do I get the platform base DPI?
    dpi_scale = win_x_scale > win_y_scale ? win_y_scale : win_x_scale;
#endif

    // Setup Dear ImGui style
    guiSetupStyle(dpi_scale);
    guiSetupFonts(io->Fonts, dpi_scale, platform->paths->data);

// Setup Platform/Renderer backends
#if SDL_BACKEND
    ImGui_ImplSDL2_InitForOpenGL(window, gl_context);
#else
    ImGui_ImplGlfw_InitForOpenGL(window, true);
#endif
    ImGui_ImplOpenGL3_Init(gl_ver.glsl);

    // Main loop
    FontOptions font_opts = {
        .freetype_enabled = true,
        .oversample_h = 1,
        .oversample_v = 1,
        .rasterizer_multiply = 1.0f,
    };

    // NOTE (Matteo): Ensure font rebuild before first frame
    AppUpdateResult update_result = {.flags = AppUpdateFlags_RebuildFonts};

#if SDL_BACKEND
    bool done = false;
    while (!done)
    {
        // Poll and handle events (inputs, window resize, etc.)
        // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if
        // dear imgui wants to use your inputs.
        // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main
        // application.
        // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to
        // your main application. Generally you may always pass all inputs to dear imgui,
        // and hide them from your application based on those two flags.

        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            ImGui_ImplSDL2_ProcessEvent(&event);
            if (event.type == SDL_QUIT) done = true;
            if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE &&
                event.window.windowID == SDL_GetWindowID(window))
                done = true;
        }
#else
    while (!glfwWindowShouldClose(window) && !(update_result.flags & AppUpdateFlags_Quit))
    {
        glfwPollEvents();
#endif
        // TODO (Matteo): Build a font atlas per-monitor (or DPI resolution)

        // Rebuild font atlas if required
        if (update_result.flags & AppUpdateFlags_RebuildFonts)
        {
            guiUpdateFonts(io->Fonts, &font_opts);
            // Re-upload font texture on the GPU
            ImGui_ImplOpenGL3_DestroyDeviceObjects();
            ImGui_ImplOpenGL3_CreateDeviceObjects();
        }

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
#if SDL_BACKEND
        ImGui_ImplSDL2_NewFrame(window);
#else
        ImGui_ImplGlfw_NewFrame();
#endif
        igNewFrame();

        // Application frame update
        update_result = app_api.update(app, &font_opts);

        // Rendering
        igRender();

#if SDL_BACKEND
        glViewport(0, 0, (i32)io->DisplaySize.x, (i32)io->DisplaySize.y);
#else
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
#endif

        Rgba clear_color = rgbaMultiplyAlpha32(update_result.back_color);
        glClearColor(clear_color.r, clear_color.g, clear_color.b, clear_color.a);
        glClear(GL_COLOR_BUFFER_BIT);

        ImGui_ImplOpenGL3_RenderDrawData(igGetDrawData());

        // Update and Render additional Platform Windows
        // (Platform functions may change the current OpenGL context, so we save/restore it
        // to make it easier to paste this code elsewhere.)
        if (io->ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
#if SDL_BACKEND
            SDL_Window *backup_current_window = SDL_GL_GetCurrentWindow();
            SDL_GLContext backup_current_context = SDL_GL_GetCurrentContext();
            igUpdatePlatformWindows();
            igRenderPlatformWindowsDefault(NULL, NULL);
            SDL_GL_MakeCurrent(backup_current_window, backup_current_context);

#else
            GLFWwindow *backup_current_context = glfwGetCurrentContext();
            igUpdatePlatformWindows();
            igRenderPlatformWindowsDefault(NULL, NULL);
            glfwMakeContextCurrent(backup_current_context);
#endif
        }

#if SDL_BACKEND
        SDL_GL_SwapWindow(window);
#else
        glfwSwapBuffers(window);
#endif
    }

    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
#if SDL_BACKEND
    ImGui_ImplSDL2_Shutdown();
#else
    ImGui_ImplGlfw_Shutdown();
#endif
    igDestroyContext(platform->gui->ctx);

#if SDL_BACKEND
    SDL_GL_DeleteContext(gl_context);
    SDL_DestroyWindow(window);
    SDL_Quit();
#else
    glfwDestroyWindow(window);
    glfwTerminate();
#endif

    app_api.destroy(app);

    return 0;
}

//------------------------------------------------------------------------------
// Internal functions
//------------------------------------------------------------------------------

static APP_CREATE(appCreateStub)
{
    CF_UNUSED(plat);
    CF_UNUSED(argc);
    CF_UNUSED(argv);
    return NULL;
}

static APP_DESTROY(appDestroyStub)
{
    CF_UNUSED(app);
    CF_ASSERT(!app, "");
}

static APP_UPDATE(appUpdateStub)
{
    CF_UNUSED(app);
    CF_UNUSED(opts);
    return (AppUpdateResult){.flags = AppUpdateFlags_Quit};
}

static void
appInit(AppApi *app, cfPlatform *platform)
{
    char path[Paths_Size] = {0};
    strPrintf(path, Paths_Size, "%s%s", platform->paths->base, platform->paths->dll_name);

    cfMemClear(app, sizeof(*app));

    void *app_lib = platform->libLoad(path);

    if (app_lib)
    {
        app->create = (AppCreateProc)platform->libLoadProc(app_lib, "appCreate");
        app->destroy = (AppDestroyProc)platform->libLoadProc(app_lib, "appDestroy");
        app->update = (AppUpdateProc)platform->libLoadProc(app_lib, "appUpdate");
    }

    if (!app->create || !app->destroy || !app->update)
    {
        app->create = appCreateStub;
        app->destroy = appDestroyStub;
        app->update = appUpdateStub;
    }
}

//------------------------------------------------------------------------------

void *
guiAlloc(Usize size, void *state)
{
    cfAllocator *alloc = state;
    Usize *buf = cfAlloc(alloc, size + sizeof(*buf));

    if (buf) *(buf++) = size;

    return buf;
}

void
guiFree(void *mem, void *state)
{
    if (mem)
    {
        cfAllocator *alloc = state;
        Usize *buf = mem;
        buf--;
        cfFree(alloc, buf, *buf + sizeof(*buf));
    }
}

static void
guiSetupStyleSizes(ImGuiStyle *style)
{
    // Main
    // **DEFAULT**
    // Borders
    style->WindowBorderSize = 1.0f;
    style->ChildBorderSize = 1.0f;
    style->PopupBorderSize = 1.0f;
    style->FrameBorderSize = 1.0f;
    style->TabBorderSize = 1.0f;
    // Rounding
    style->WindowRounding = 0.0f;
    style->ChildRounding = 0.0f;
    style->FrameRounding = 2.0f;
    style->PopupRounding = 2.0f;
    style->ScrollbarRounding = 4.0f;
    style->GrabRounding = style->FrameRounding;
    style->LogSliderDeadzone = 3.0f;
    style->TabRounding = 4.0f;
    // Alignment
    // **DEFAULT**
}

static void
guiSetupStyleColors(ImGuiStyle *style)
{
    ImVec4 *colors = style->Colors;

    colors[ImGuiCol_Text] = (ImVec4){1.00f, 1.00f, 1.00f, 1.00f};
    colors[ImGuiCol_TextDisabled] = (ImVec4){0.50f, 0.50f, 0.50f, 1.00f};
    colors[ImGuiCol_WindowBg] = (ImVec4){0.06f, 0.06f, 0.06f, 0.94f};
    colors[ImGuiCol_ChildBg] = (ImVec4){0.00f, 0.00f, 0.00f, 0.00f};
    colors[ImGuiCol_PopupBg] = (ImVec4){0.08f, 0.08f, 0.08f, 0.94f};
    colors[ImGuiCol_Border] = (ImVec4){0.43f, 0.43f, 0.50f, 0.50f};
    colors[ImGuiCol_BorderShadow] = (ImVec4){0.00f, 0.00f, 0.00f, 0.00f};
    colors[ImGuiCol_FrameBg] = (ImVec4){0.44f, 0.44f, 0.44f, 0.60f};
    colors[ImGuiCol_FrameBgHovered] = (ImVec4){0.57f, 0.57f, 0.57f, 0.70f};
    colors[ImGuiCol_FrameBgActive] = (ImVec4){0.76f, 0.76f, 0.76f, 0.80f};
    colors[ImGuiCol_TitleBg] = (ImVec4){0.04f, 0.04f, 0.04f, 1.00f};
    colors[ImGuiCol_TitleBgActive] = (ImVec4){0.16f, 0.16f, 0.16f, 1.00f};
    colors[ImGuiCol_TitleBgCollapsed] = (ImVec4){0.00f, 0.00f, 0.00f, 0.60f};
    colors[ImGuiCol_MenuBarBg] = (ImVec4){0.14f, 0.14f, 0.14f, 1.00f};
    colors[ImGuiCol_ScrollbarBg] = (ImVec4){0.02f, 0.02f, 0.02f, 0.53f};
    colors[ImGuiCol_ScrollbarGrab] = (ImVec4){0.31f, 0.31f, 0.31f, 1.00f};
    colors[ImGuiCol_ScrollbarGrabHovered] = (ImVec4){0.41f, 0.41f, 0.41f, 1.00f};
    colors[ImGuiCol_ScrollbarGrabActive] = (ImVec4){0.51f, 0.51f, 0.51f, 1.00f};
    colors[ImGuiCol_CheckMark] = (ImVec4){0.13f, 0.75f, 0.55f, 0.80f};
    colors[ImGuiCol_SliderGrab] = (ImVec4){0.13f, 0.75f, 0.75f, 0.80f};
    colors[ImGuiCol_SliderGrabActive] = (ImVec4){0.13f, 0.75f, 1.00f, 0.80f};
    colors[ImGuiCol_Button] = (ImVec4){0.13f, 0.75f, 0.55f, 0.40f};
    colors[ImGuiCol_ButtonHovered] = (ImVec4){0.13f, 0.75f, 0.75f, 0.60f};
    colors[ImGuiCol_ButtonActive] = (ImVec4){0.13f, 0.75f, 1.00f, 0.80f};
    colors[ImGuiCol_Header] = (ImVec4){0.13f, 0.75f, 0.55f, 0.40f};
    colors[ImGuiCol_HeaderHovered] = (ImVec4){0.13f, 0.75f, 0.75f, 0.60f};
    colors[ImGuiCol_HeaderActive] = (ImVec4){0.13f, 0.75f, 1.00f, 0.80f};
    colors[ImGuiCol_Separator] = (ImVec4){0.13f, 0.75f, 0.55f, 0.40f};
    colors[ImGuiCol_SeparatorHovered] = (ImVec4){0.13f, 0.75f, 0.75f, 0.60f};
    colors[ImGuiCol_SeparatorActive] = (ImVec4){0.13f, 0.75f, 1.00f, 0.80f};
    colors[ImGuiCol_ResizeGrip] = (ImVec4){0.13f, 0.75f, 0.55f, 0.40f};
    colors[ImGuiCol_ResizeGripHovered] = (ImVec4){0.13f, 0.75f, 0.75f, 0.60f};
    colors[ImGuiCol_ResizeGripActive] = (ImVec4){0.13f, 0.75f, 1.00f, 0.80f};
    colors[ImGuiCol_Tab] = (ImVec4){0.13f, 0.75f, 0.55f, 0.80f};
    colors[ImGuiCol_TabHovered] = (ImVec4){0.13f, 0.75f, 0.75f, 0.80f};
    colors[ImGuiCol_TabActive] = (ImVec4){0.13f, 0.75f, 1.00f, 0.80f};
    colors[ImGuiCol_TabUnfocused] = (ImVec4){0.18f, 0.18f, 0.18f, 1.00f};
    colors[ImGuiCol_TabUnfocusedActive] = (ImVec4){0.36f, 0.36f, 0.36f, 0.54f};
    colors[ImGuiCol_DockingPreview] = (ImVec4){0.13f, 0.75f, 0.55f, 0.80f};
    colors[ImGuiCol_DockingEmptyBg] = (ImVec4){0.13f, 0.13f, 0.13f, 0.80f};
    colors[ImGuiCol_PlotLines] = (ImVec4){0.61f, 0.61f, 0.61f, 1.00f};
    colors[ImGuiCol_PlotLinesHovered] = (ImVec4){1.00f, 0.43f, 0.35f, 1.00f};
    colors[ImGuiCol_PlotHistogram] = (ImVec4){0.90f, 0.70f, 0.00f, 1.00f};
    colors[ImGuiCol_PlotHistogramHovered] = (ImVec4){1.00f, 0.60f, 0.00f, 1.00f};
    colors[ImGuiCol_TableHeaderBg] = (ImVec4){0.19f, 0.19f, 0.20f, 1.00f};
    colors[ImGuiCol_TableBorderStrong] = (ImVec4){0.31f, 0.31f, 0.35f, 1.00f};
    colors[ImGuiCol_TableBorderLight] = (ImVec4){0.23f, 0.23f, 0.25f, 1.00f};
    colors[ImGuiCol_TableRowBg] = (ImVec4){0.00f, 0.00f, 0.00f, 0.00f};
    colors[ImGuiCol_TableRowBgAlt] = (ImVec4){1.00f, 1.00f, 1.00f, 0.07f};
    colors[ImGuiCol_TextSelectedBg] = (ImVec4){0.26f, 0.59f, 0.98f, 0.35f};
    colors[ImGuiCol_DragDropTarget] = (ImVec4){1.00f, 1.00f, 0.00f, 0.90f};
    colors[ImGuiCol_NavHighlight] = (ImVec4){0.26f, 0.59f, 0.98f, 1.00f};
    colors[ImGuiCol_NavWindowingHighlight] = (ImVec4){1.00f, 1.00f, 1.00f, 0.70f};
    colors[ImGuiCol_NavWindowingDimBg] = (ImVec4){0.80f, 0.80f, 0.80f, 0.20f};
    colors[ImGuiCol_ModalWindowDimBg] = (ImVec4){0.80f, 0.80f, 0.80f, 0.35f};
}

void
guiSetupStyle(F32 dpi_scale)
{
    igStyleColorsClassic(NULL);

    // When viewports are enabled we tweak WindowRounding/WindowBg so platform
    // windows can look identical to regular ones.
    ImGuiStyle *style = igGetStyle();

    if (igGetIO()->ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        style->WindowRounding = 0.0f;
        style->Colors[ImGuiCol_WindowBg].w = 1.0f;
    }

    guiSetupStyleSizes(style);
    guiSetupStyleColors(style);

    ImGuiStyle_ScaleAllSizes(style, dpi_scale);
}

ImFont *
guiLoadFont(ImFontAtlas *fonts, char const *data_path, char const *name, F32 font_size,
            ImWchar const *ranges)
{
    char buffer[1024] = {0};

    snprintf(buffer, 1024, "%s%s.ttf", data_path, name);

    return ImFontAtlas_AddFontFromFileTTF(fonts, buffer, font_size, NULL, ranges);
}

void
guiSetupFonts(ImFontAtlas *fonts, F32 dpi_scale, char const *data_path)
{
    // Load Fonts
    // - If no fonts are loaded, dear imgui will use the default font. You can
    // also load multiple fonts and use igPushFont()/PopFont() to select them.
    // - AddFontFromFileTTF() will return the ImFont* so you can store it if you
    // need to select the font among multiple.
    // - If the file cannot be loaded, the function will return NULL. Please
    // handle those errors in your application (e.g. use an assertion, or
    // display an error and quit).
    // - The fonts will be rasterized at a given size (w/ oversampling) and
    // stored into a texture when calling
    // ImFontAtlas::Build()/GetTexDataAsXXXX(), which ImGui_ImplXXXX_NewFrame
    // below will call.
    // - Read 'docs/FONTS.md' for more instructions and details.
    // - Remember that in C/C++ if you want to include a backslash \ in a string
    // literal you need to write a double backslash \\ !
    ImWchar const *ranges = ImFontAtlas_GetGlyphRangesDefault(fonts);

    F32 const scale = dpi_scale * PLATFORM_DPI / TRUETYPE_DPI;

    if (!guiLoadFont(fonts, data_path, "NotoSans", cfRound(13.0f * scale), ranges) &
        !guiLoadFont(fonts, data_path, "OpenSans", cfRound(13.5f * scale), ranges) &
        !guiLoadFont(fonts, data_path, "SourceSansPro", cfRound(13.5f * scale), ranges) &
        !guiLoadFont(fonts, data_path, "DroidSans", cfRound(12.0f * scale), ranges))
    {
        ImFontAtlas_AddFontDefault(fonts, NULL);
    }
}

void
guiUpdateFonts(ImFontAtlas *fonts, FontOptions *font_opts)
{
    if (font_opts->tex_glyph_padding != 0)
    {
        fonts->TexGlyphPadding = font_opts->tex_glyph_padding;
    }

    for (I32 i = 0; i < fonts->ConfigData.Size; ++i)
    {
        fonts->ConfigData.Data[i].RasterizerMultiply = font_opts->rasterizer_multiply;
        fonts->ConfigData.Data[i].OversampleH = font_opts->oversample_h;
        fonts->ConfigData.Data[i].OversampleV = font_opts->oversample_v;
    }

    if (font_opts->freetype_enabled)
    {
        fonts->FontBuilderIO = ImGuiFreeType_GetBuilderForFreeType();
        fonts->FontBuilderFlags = (U32)font_opts->freetype_flags;
    }
    else
    {
        fonts->FontBuilderIO = igImFontAtlasGetBuilderForStbTruetype();
    }

    ImFontAtlas_Build(fonts);

    font_opts->tex_glyph_padding = fonts->TexGlyphPadding;
}

//------------------------------------------------------------------------------
