// Interface between platform layer and hosted applicatiom
#include "app.h"

// Gui library
#include "gui/gui.h"

// Backend libraries
#include "gl/gload.h"
#include <GLFW/glfw3.h>

// Foundation library
#include "foundation/colors.h"
#include "foundation/core.h"
#include "foundation/fs.h"
#include "foundation/math.inl"
#include "foundation/memory.h"
#include "foundation/paths.h"
#include "foundation/strings.h"

// Standard library
// TODO (Matteo): Get rid of it (currently here only for stderr)
#include <stdio.h>

//------------------------------------------------------------------------------
// OS specific platform layer
//------------------------------------------------------------------------------

// NOTE (Matteo): This is where the actual entry point is

#if CF_OS_WIN32
#    include "platform_win32.c"
#else
#    error "OS specific layer not implemented"
#endif

//------------------------------------------------------------------------------
// Local function declarations
//------------------------------------------------------------------------------

// TODO (Matteo): Better logging
#define logError(...) fprintf(stderr, __VA_ARGS__)

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
    Cstr glsl;
} GlVersion;

//------------------------------------------------------------------------------
// GLFW backend declarations
//------------------------------------------------------------------------------

extern bool ImGui_ImplGlfw_InitForOpenGL(GLFWwindow *window, bool install_callbacks);
extern void ImGui_ImplGlfw_Shutdown();
extern void ImGui_ImplGlfw_NewFrame();

static void
glfwErrorCallback(int error, Cstr description)
{
    logError("Glfw Error %d: %s\n", error, description);
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
glfwCreateWinAndContext(Cstr title, I32 width, I32 height, GlVersion *gl_version)
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
    Char8 src_file[Paths_Size];
    Char8 dst_file[Paths_Size];
} AppApi;

static APP_PROC(appProcStub);
static APP_CREATE_PROC(appCreateStub);
static APP_UPDATE_PROC(appUpdateStub);

static void appApiLoad(AppApi *api, Paths *paths, LibraryApi *library);
static void appApiUpdate(AppApi *api, Platform *platform, AppState *app);

//------------------------------------------------------------------------------
// Main
//------------------------------------------------------------------------------

static void
platformUpdateFullscreen(GLFWwindow *window, bool fullscreen, IVec2 *win_pos, IVec2 *win_size)
{
    // TODO (Matteo): Investigate freeze when leaving fullscreen mode
    GLFWmonitor *monitor = glfwGetWindowMonitor(window);
    bool is_fullscreen = (monitor != NULL);
    if (fullscreen != is_fullscreen)
    {
        if (is_fullscreen)
        {
            glfwSetWindowMonitor(window, NULL,           //
                                 win_pos->x, win_pos->y, //
                                 win_size->width, win_size->height, 0);
        }
        else
        {
            glfwGetWindowPos(window, &win_pos->x, &win_pos->y);
            glfwGetWindowSize(window, &win_size->width, &win_size->height);
            monitor = glfwGetPrimaryMonitor();
            GLFWvidmode const *vidmode = glfwGetVideoMode(monitor);
            glfwSetWindowMonitor(window, monitor, 0, 0, vidmode->width, vidmode->height,
                                 vidmode->refreshRate);
        }
    }
}

#if PLATFORM_DPI_HANDLING

static bool
platformUpdateMainDpi(ImFontAtlas *fonts, F32 *curr_dpi, Str data_path)
{
    ImGuiViewport *vp = igGetMainViewport();
    ImGuiPlatformMonitor const *mon = igGetViewportPlatformMonitor(vp);

    if (mon->DpiScale != *curr_dpi)
    {
        F32 ratio = mon->DpiScale / *curr_dpi;
        *curr_dpi = mon->DpiScale;
#    if 1
        ImFontAtlas_Clear(fonts);
        if (!guiLoadCustomFonts(fonts, *curr_dpi, data_path))
        {
            guiLoadDefaultFont(fonts);
        }
        // TODO (Matteo): Should rescale size?
        CF_UNUSED(ratio); // ImGuiStyle_ScaleAllSizes(igGetStyle(), ratio);
        return true;
#    else
        CF_UNUSED(data_path);
        io->FontGlobalScale *= ratio;
#    endif
    }

    return false;
}

#endif

I32
platformMain(Platform *platform, CommandLine *cmd_line)
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
        logError("Failed to initialize OpenGL loader!\n");
        return -2;
    }

    // Setup platform layer
    CF_ASSERT_NOT_NULL(gl);
    clockStart(&platform->clock);
    platform->gl = gl;

// NOTE (Matteo): Custom IMGUI ini file
// TODO (Matteo): Clean up!
#if CF_COMPILER_MSVC
#    pragma warning(push)
#    pragma warning(disable : 4221)
#endif
    Char8 gui_ini[Paths_Size] = {0};
    CF_ASSERT(paths->base.len + paths->exe_name.len < Paths_Size, "IMGUI ini file name too long");
    memCopy(paths->base.buf, gui_ini, paths->base.len);
    memCopy(paths->exe_name.buf, gui_ini + paths->base.len, paths->exe_name.len);
    pathChangeExt(strFromCstr(gui_ini), strLiteral(".gui"), gui_ini);

    // Setup Dear ImGui context
    platform->gui = &(Gui){
        .alloc = platform->heap,
        .ini_filename = gui_ini,
    };
    guiInit(platform->gui);
#if CF_COMPILER_MSVC
#    pragma warning(pop)
#endif

    // Setup application
    AppApi app_api = {0};
    appApiLoad(&app_api, platform->paths, platform->library);
    AppState *app_state = app_api.create(platform, cmd_line);

    // Setup DPI handling
    F32 win_x_scale, win_y_scale;
    glfwGetWindowContentScale(window, &win_x_scale, &win_y_scale);
    // HACK How do I get the platform base DPI?
    F32 dpi_scale = win_x_scale > win_y_scale ? win_y_scale : win_x_scale;

    // Setup Dear ImGui style
    guiSetupStyle(GuiTheme_Dark, dpi_scale);
    if (!guiLoadCustomFonts(guiFonts(), dpi_scale, paths->data))
    {
        guiLoadDefaultFont(guiFonts());
    }

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(gl_ver.glsl);

    // Main loop
    IVec2 win_pos, win_size;
    AppIo app_io = {
        .font_opts =
            &(GuiFontOptions){
                .freetype_enabled = true,
                .oversample_h = 1,
                .oversample_v = 1,
                .rasterizer_multiply = 1.0f,
            },
        // NOTE (Matteo): Ensure font rebuild before first frame
        .rebuild_fonts = true,
        // NOTE (Matteo): Ensure fast first update
        .continuous_update = true,
    };

    while (!glfwWindowShouldClose(window) && !app_io.quit)
    {
        //------------------//
        // Event processing //
        //------------------//

        // TODO (Matteo): Maybe improve a bit?
        if (app_io.window_title_changed)
        {
            glfwSetWindowTitle(window, app_io.window_title);
        }

        if (app_io.continuous_update)
        {
            glfwPollEvents();
        }
        else
        {
            glfwWaitEvents();
        }

        // NOTE (Matteo): Auto reloading of application library
        appApiUpdate(&app_api, platform, app_state);

        platformUpdateFullscreen(window, app_io.fullscreen, &win_pos, &win_size);

        // Rebuild font atlas if required
        if (app_io.rebuild_fonts)
        {
            app_io.rebuild_fonts = false;
            guiUpdateAtlas(guiFonts(), app_io.font_opts);
            // Re-upload font texture on the GPU
            ImGui_ImplOpenGL3_DestroyDeviceObjects();
            ImGui_ImplOpenGL3_CreateDeviceObjects();
        }

        //--------------//
        // Frame update //
        //--------------//

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        guiNewFrame();

        // NOTE (Matteo): Setup GL viewport and clear buffers BEFORE app update in order to allow
        // the application code to draw directly using OpenGL
        Rgba clear_color = rgbaMultiplyAlpha32(app_io.back_color);
        IVec2 display = {0};
        glfwGetFramebufferSize(window, &display.width, &display.height);
        glViewport(0, 0, display.width, display.height);
        glClearColor(clear_color.r, clear_color.g, clear_color.b, clear_color.a);
        glClear(GL_COLOR_BUFFER_BIT);

        // Application frame update
        app_api.update(app_state, &app_io);

        //-----------//
        // Rendering //
        //-----------//

        ImDrawData *draw_data = guiRender();
        ImGui_ImplOpenGL3_RenderDrawData(draw_data);

        // Update and Render additional Platform Windows
        // (Platform functions may change the current OpenGL context, so we save/restore it
        // to make it easier to paste this code elsewhere.)
        if (guiViewportsEnabled())
        {
            GLFWwindow *backup_current_context = glfwGetCurrentContext();
            guiUpdateAndRenderViewports();
            glfwMakeContextCurrent(backup_current_context);
        }

        glfwSwapBuffers(window);

        //-----------------//
        // Post processing //
        //-----------------//

#if PLATFORM_DPI_HANDLING
        // NOTE (Matteo): Simple DPI handling for main viewport
        // TODO (Matteo): Build a font atlas per-monitor (or DPI resolution)
        if (platformUpdateMainDpi(io, &dpi_scale, paths->data))
        {
            app_io.rebuild_fonts = true;
        }
#endif
    }

    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    guiShutdown(platform->gui);

    glfwDestroyWindow(window);
    glfwTerminate();

    app_api.destroy(app_state);

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
    CF_UNUSED(cmd_line);
    return NULL;
}

static APP_UPDATE_PROC(appUpdateStub)
{
    CF_UNUSED(state);
    io->quit = true;
}

void
appApiLoad(AppApi *api, Paths *paths, LibraryApi *library)
{
    if (api->lib)
    {
        CF_ASSERT(api->create, "");
        library->unload(api->lib);
        memClearStruct(api);
    }

    strPrintf(api->src_file, Paths_Size, "%.*s%.*s", //
              (I32)paths->base.len, paths->base.buf, //
              (I32)paths->lib_name.len, paths->lib_name.buf);
    strPrintf(api->dst_file, Paths_Size, "%s.tmp", api->src_file);

    Str dst_file = strFromCstr(api->dst_file);
    if (fileCopy(strFromCstr(api->src_file), dst_file, true))
    {
        api->lib = library->load(dst_file);
    }

    if (api->lib)
    {
        api->create = (AppCreateProc)library->loadSymbol(api->lib, "appCreate");
        api->destroy = (AppProc)library->loadSymbol(api->lib, "appDestroy");
        api->load = (AppProc)library->loadSymbol(api->lib, "appLoad");
        api->unload = (AppProc)library->loadSymbol(api->lib, "appUnload");
        api->update = (AppUpdateProc)library->loadSymbol(api->lib, "appUpdate");
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
    if (fileProperties(strFromCstr(api->src_file)).last_write >
        fileProperties(strFromCstr(api->dst_file)).last_write)
    {
        api->unload(app);
        appApiLoad(api, platform->paths, platform->library);
        api->load(app);
    }
}

//------------------------------------------------------------------------------
