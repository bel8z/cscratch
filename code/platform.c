// Interface between platform layer and hosted applicatiom
#include "api.h"

// Gui library
#include "gui/gui.h"

// Backend libraries
#include "gl/gload.h"
#include <GLFW/glfw3.h>

// Foundation library
#include "foundation/core.h"

#include "foundation/color.h"
#include "foundation/fs.h"
#include "foundation/paths.h"
#include "foundation/strings.h"

// Standard library
#include <stdio.h>

// Constants for DPI handling
#define PLATFORM_DPI 96.0f
#define TRUETYPE_DPI 72.0f

//------------------------------------------------------------------------------
// Local function declarations
//------------------------------------------------------------------------------

static void guiSetupStyle(F32 dpi);
static void guiSetupFonts(ImFontAtlas *fonts, F32 dpi, Str data_path);

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

//------------------------------------------------------------------------------
// Application API
//------------------------------------------------------------------------------

typedef struct AppApi
{
    Library *lib;

    AppCreateProc create;
    AppProc destroy;

    AppProc load;
    AppProc unload;

    AppUpdateProc update;

    // TODO (Matteo): reduce redundancy
    char src_file[Paths_Size];
    char dst_file[Paths_Size];
} AppApi;

static APP_PROC(appProcStub);
static APP_CREATE_PROC(appCreateStub);
static APP_UPDATE_PROC(appUpdateStub);

static void appApiLoad(AppApi *api, Platform *platform);
static void appApiUpdate(AppApi *api, Platform *platform, AppState *app);

//------------------------------------------------------------------------------
// Main
//------------------------------------------------------------------------------

I32
platformMain(Platform *platform, char const *argv[], I32 argc)
{
    Paths *paths = platform->paths;

    // Setup window
    glfwSetErrorCallback(glfwErrorCallback);
    if (!glfwInit()) return 1;

    // Create window with graphics context
    GlVersion gl_ver = {0};
    GLFWwindow *window = glfwCreateWinAndContext("Dear ImGui template", 1280, 720, &gl_ver);
    if (window == NULL) return 1;
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Enable vsync

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
    platform->gui = &(Gui){.alloc = &platform->heap};
    guiInit(platform->gui);

    // Setup application
    AppApi app_api = {0};
    appApiLoad(&app_api, platform);
    AppState *app = app_api.create(platform, argv, argc);

    // Configure Dear ImGui
    ImGuiIO *io = igGetIO();

// NOTE (Matteo): Custom IMGUI ini file
// TODO (Matteo): Clean up!
#if CF_COMPILER_MSVC
#    pragma warning(push)
#    pragma warning(disable : 4221)
#endif
    char gui_ini[Paths_Size] = {0};
    CF_ASSERT(paths->base.len + paths->exe_name.len < Paths_Size, "IMGUI ini file name too long");
    cfMemCopy(paths->base.buf, gui_ini, paths->base.len);
    cfMemCopy(paths->exe_name.buf, gui_ini + paths->base.len, paths->exe_name.len);
    pathChangeExt(strFromCstr(gui_ini), strFromCstr(".gui"), gui_ini);
    io->IniFilename = gui_ini;
#if CF_COMPILER_MSVC
#    pragma warning(pop)
#endif

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

    F32 win_x_scale, win_y_scale;
    glfwGetWindowContentScale(window, &win_x_scale, &win_y_scale);
    // HACK How do I get the platform base DPI?
    dpi_scale = win_x_scale > win_y_scale ? win_y_scale : win_x_scale;

    // Setup Dear ImGui style
    guiSetupStyle(dpi_scale);
    guiSetupFonts(io->Fonts, dpi_scale, paths->data);

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(window, true);
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
        // NOTE (Matteo): Auto reloading of application library
        appApiUpdate(&app_api, platform, app);

        // TODO (Matteo): Build a font atlas per-monitor (or DPI resolution)

        // Rebuild font atlas if required
        if (update_result.flags & AppUpdateFlags_RebuildFonts)
        {
            guiUpdateAtlas(io->Fonts, &font_opts);
            // Re-upload font texture on the GPU
            ImGui_ImplOpenGL3_DestroyDeviceObjects();
            ImGui_ImplOpenGL3_CreateDeviceObjects();
        }

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        igNewFrame();

        // Application frame update
        update_result = app_api.update(app, &font_opts);

        // Rendering
        igRender();

        I32 display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);

        Rgba clear_color = rgbaMultiplyAlpha32(update_result.back_color);
        glClearColor(clear_color.r, clear_color.g, clear_color.b, clear_color.a);
        glClear(GL_COLOR_BUFFER_BIT);

        ImGui_ImplOpenGL3_RenderDrawData(igGetDrawData());

        // Update and Render additional Platform Windows
        // (Platform functions may change the current OpenGL context, so we save/restore it
        // to make it easier to paste this code elsewhere.)
        if (io->ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            GLFWwindow *backup_current_context = glfwGetCurrentContext();
            igUpdatePlatformWindows();
            igRenderPlatformWindowsDefault(NULL, NULL);
            glfwMakeContextCurrent(backup_current_context);
        }

        glfwSwapBuffers(window);
    }

    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    igDestroyContext(platform->gui->ctx);

    glfwDestroyWindow(window);
    glfwTerminate();

    app_api.destroy(app);

    return 0;
}

//------------------------------------------------------------------------------
// Internal functions
//------------------------------------------------------------------------------

static APP_PROC(appProcStub)
{
    CF_UNUSED(app);
}

static APP_CREATE_PROC(appCreateStub)
{
    CF_UNUSED(plat);
    CF_UNUSED(argc);
    CF_UNUSED(argv);
    return NULL;
}

static APP_UPDATE_PROC(appUpdateStub)
{
    CF_UNUSED(app);
    CF_UNUSED(opts);
    return (AppUpdateResult){.flags = AppUpdateFlags_Quit};
}

void
appApiLoad(AppApi *api, Platform *platform)
{
    Paths *paths = platform->paths;

    if (api->lib)
    {
        CF_ASSERT(api->create, "");
        platform->libUnload(api->lib);
        cfMemClear(api, sizeof(*api));
    }

    strPrintf(api->src_file, Paths_Size, "%.*s%.*s", //
              paths->base.len, paths->base.buf,      //
              paths->lib_name.len, paths->lib_name.buf);
    strPrintf(api->dst_file, Paths_Size, "%s.tmp", api->src_file);

    if (platform->fs->fileCopy(strFromCstr(api->src_file), strFromCstr(api->dst_file), true))
    {
        api->lib = platform->libLoad(api->dst_file);
    }

    if (api->lib)
    {
        api->create = (AppCreateProc)platform->libLoadProc(api->lib, "appCreate");
        api->destroy = (AppProc)platform->libLoadProc(api->lib, "appDestroy");
        api->load = (AppProc)platform->libLoadProc(api->lib, "appLoad");
        api->unload = (AppProc)platform->libLoadProc(api->lib, "appUnload");
        api->update = (AppUpdateProc)platform->libLoadProc(api->lib, "appUpdate");
    }

    if (!api->create) api->create = appCreateStub;
    if (!api->destroy) api->destroy = appProcStub;
    if (!api->load) api->load = appProcStub;
    if (!api->unload) api->unload = appProcStub;
    if (!api->update) api->update = appUpdateStub;
}

void
appApiUpdate(AppApi *api, Platform *platform, AppState *app)
{
    // TODO (Matteo): Are these operation too expensive to be performed every frame?
    if (platform->fs->fileWriteTime(strFromCstr(api->src_file)) >
        platform->fs->fileWriteTime(strFromCstr(api->dst_file)))
    {
        api->unload(app);
        appApiLoad(api, platform);
        api->load(app);
    }
}

//------------------------------------------------------------------------------

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

    // clang-format off
    colors[ImGuiCol_Text]                   = (ImVec4){1.00f, 1.00f, 1.00f, 1.00f};
    colors[ImGuiCol_TextDisabled]           = (ImVec4){0.50f, 0.50f, 0.50f, 1.00f};
    colors[ImGuiCol_WindowBg]               = (ImVec4){0.06f, 0.06f, 0.06f, 0.94f};
    colors[ImGuiCol_ChildBg]                = (ImVec4){0.00f, 0.00f, 0.00f, 0.00f};
    colors[ImGuiCol_PopupBg]                = (ImVec4){0.08f, 0.08f, 0.08f, 0.94f};
    colors[ImGuiCol_Border]                 = (ImVec4){0.43f, 0.43f, 0.50f, 0.50f};
    colors[ImGuiCol_BorderShadow]           = (ImVec4){0.00f, 0.00f, 0.00f, 0.00f};
    colors[ImGuiCol_FrameBg]                = (ImVec4){0.44f, 0.44f, 0.44f, 0.60f};
    colors[ImGuiCol_FrameBgHovered]         = (ImVec4){0.57f, 0.57f, 0.57f, 0.70f};
    colors[ImGuiCol_FrameBgActive]          = (ImVec4){0.76f, 0.76f, 0.76f, 0.80f};
    colors[ImGuiCol_TitleBg]                = (ImVec4){0.04f, 0.04f, 0.04f, 1.00f};
    colors[ImGuiCol_TitleBgActive]          = (ImVec4){0.16f, 0.16f, 0.16f, 1.00f};
    colors[ImGuiCol_TitleBgCollapsed]       = (ImVec4){0.00f, 0.00f, 0.00f, 0.60f};
    colors[ImGuiCol_MenuBarBg]              = (ImVec4){0.14f, 0.14f, 0.14f, 1.00f};
    colors[ImGuiCol_ScrollbarBg]            = (ImVec4){0.02f, 0.02f, 0.02f, 0.53f};
    colors[ImGuiCol_ScrollbarGrab]          = (ImVec4){0.31f, 0.31f, 0.31f, 1.00f};
    colors[ImGuiCol_ScrollbarGrabHovered]   = (ImVec4){0.41f, 0.41f, 0.41f, 1.00f};
    colors[ImGuiCol_ScrollbarGrabActive]    = (ImVec4){0.51f, 0.51f, 0.51f, 1.00f};
    colors[ImGuiCol_CheckMark]              = (ImVec4){0.13f, 0.75f, 0.55f, 0.80f};
    colors[ImGuiCol_SliderGrab]             = (ImVec4){0.13f, 0.75f, 0.75f, 0.80f};
    colors[ImGuiCol_SliderGrabActive]       = (ImVec4){0.13f, 0.75f, 1.00f, 0.80f};
    colors[ImGuiCol_Button]                 = (ImVec4){0.13f, 0.75f, 0.55f, 0.40f};
    colors[ImGuiCol_ButtonHovered]          = (ImVec4){0.13f, 0.75f, 0.75f, 0.60f};
    colors[ImGuiCol_ButtonActive]           = (ImVec4){0.13f, 0.75f, 1.00f, 0.80f};
    colors[ImGuiCol_Header]                 = (ImVec4){0.13f, 0.75f, 0.55f, 0.40f};
    colors[ImGuiCol_HeaderHovered]          = (ImVec4){0.13f, 0.75f, 0.75f, 0.60f};
    colors[ImGuiCol_HeaderActive]           = (ImVec4){0.13f, 0.75f, 1.00f, 0.80f};
    colors[ImGuiCol_Separator]              = (ImVec4){0.13f, 0.75f, 0.55f, 0.40f};
    colors[ImGuiCol_SeparatorHovered]       = (ImVec4){0.13f, 0.75f, 0.75f, 0.60f};
    colors[ImGuiCol_SeparatorActive]        = (ImVec4){0.13f, 0.75f, 1.00f, 0.80f};
    colors[ImGuiCol_ResizeGrip]             = (ImVec4){0.13f, 0.75f, 0.55f, 0.40f};
    colors[ImGuiCol_ResizeGripHovered]      = (ImVec4){0.13f, 0.75f, 0.75f, 0.60f};
    colors[ImGuiCol_ResizeGripActive]       = (ImVec4){0.13f, 0.75f, 1.00f, 0.80f};
    colors[ImGuiCol_Tab]                    = (ImVec4){0.13f, 0.75f, 0.55f, 0.80f};
    colors[ImGuiCol_TabHovered]             = (ImVec4){0.13f, 0.75f, 0.75f, 0.80f};
    colors[ImGuiCol_TabActive]              = (ImVec4){0.13f, 0.75f, 1.00f, 0.80f};
    colors[ImGuiCol_TabUnfocused]           = (ImVec4){0.18f, 0.18f, 0.18f, 1.00f};
    colors[ImGuiCol_TabUnfocusedActive]     = (ImVec4){0.36f, 0.36f, 0.36f, 0.54f};
    colors[ImGuiCol_DockingPreview]         = (ImVec4){0.13f, 0.75f, 0.55f, 0.80f};
    colors[ImGuiCol_DockingEmptyBg]         = (ImVec4){0.13f, 0.13f, 0.13f, 0.80f};
    colors[ImGuiCol_PlotLines]              = (ImVec4){0.61f, 0.61f, 0.61f, 1.00f};
    colors[ImGuiCol_PlotLinesHovered]       = (ImVec4){1.00f, 0.43f, 0.35f, 1.00f};
    colors[ImGuiCol_PlotHistogram]          = (ImVec4){0.90f, 0.70f, 0.00f, 1.00f};
    colors[ImGuiCol_PlotHistogramHovered]   = (ImVec4){1.00f, 0.60f, 0.00f, 1.00f};
    colors[ImGuiCol_TableHeaderBg]          = (ImVec4){0.19f, 0.19f, 0.20f, 1.00f};
    colors[ImGuiCol_TableBorderStrong]      = (ImVec4){0.31f, 0.31f, 0.35f, 1.00f};
    colors[ImGuiCol_TableBorderLight]       = (ImVec4){0.23f, 0.23f, 0.25f, 1.00f};
    colors[ImGuiCol_TableRowBg]             = (ImVec4){0.00f, 0.00f, 0.00f, 0.00f};
    colors[ImGuiCol_TableRowBgAlt]          = (ImVec4){1.00f, 1.00f, 1.00f, 0.07f};
    colors[ImGuiCol_TextSelectedBg]         = (ImVec4){0.26f, 0.59f, 0.98f, 0.35f};
    colors[ImGuiCol_DragDropTarget]         = (ImVec4){1.00f, 1.00f, 0.00f, 0.90f};
    colors[ImGuiCol_NavHighlight]           = (ImVec4){0.26f, 0.59f, 0.98f, 1.00f};
    colors[ImGuiCol_NavWindowingHighlight]  = (ImVec4){1.00f, 1.00f, 1.00f, 0.70f};
    colors[ImGuiCol_NavWindowingDimBg]      = (ImVec4){0.80f, 0.80f, 0.80f, 0.20f};
    colors[ImGuiCol_ModalWindowDimBg]       = (ImVec4){0.80f, 0.80f, 0.80f, 0.35f};
    // clang-format on
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
guiLoadFont(ImFontAtlas *fonts, Str data_path, char const *name, F32 font_size,
            ImWchar const *ranges)
{
    char buffer[1024] = {0};
    snprintf(buffer, 1024, "%.*s%s.ttf", (I32)data_path.len, data_path.buf, name);
    return ImFontAtlas_AddFontFromFileTTF(fonts, buffer, font_size, NULL, ranges);
}

void
guiSetupFonts(ImFontAtlas *fonts, F32 dpi_scale, Str data_path)
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

//------------------------------------------------------------------------------
