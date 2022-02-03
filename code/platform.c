// Interface between platform layer and hosted applicatiom
#include "app.h"

// Gui library
#include "gui/gui.h"
#include "gui/gui_backend_gl3.h"
#include "gui/win.h"

// Backend libraries
#include "gl/gload.h"

// Foundation library
#include "foundation/colors.h"
#include "foundation/core.h"
#include "foundation/io.h"
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

static APP_FN(appProcStub);
static APP_CREATE_FN(appCreateStub);
static APP_UPDATE_FN(appUpdateStub);

static void appApiLoad(AppApi *api, Paths *paths, LibraryApi *library, IoFileApi *file);
static void appApiUpdate(AppApi *api, Platform *platform, AppState *app);

//------------------------------------------------------------------------------
// Main
//------------------------------------------------------------------------------

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

    // Setup platform layer
    clockStart(&platform->clock);

    // NOTE (Matteo): Custom IMGUI ini file
    // TODO (Matteo): Clean up!

    CF_DIAGNOSTIC_PUSH()
    CF_DIAGNOSTIC_IGNORE_MSVC(4221)

    Char8 gui_ini[Paths_Size] = {0};
    CF_ASSERT(paths->base.len + paths->exe_name.len < Paths_Size, "IMGUI ini file name too long");
    memCopy(paths->base.buf, gui_ini, paths->base.len);
    memCopy(paths->exe_name.buf, gui_ini + paths->base.len, paths->exe_name.len);
    pathChangeExt(strFromCstr(gui_ini), strLiteral(".gui"), gui_ini);

    // Setup Dear ImGui context
    GuiOpenGLVersion gl_ver = {0};
    platform->gui = guiInit(
        &(GuiInitInfo){
            .alloc = platform->heap,
            .ini_filename = gui_ini,
            .data_path = paths->data,
            .gl_context = true,
        },
        &gl_ver);

    if (!platform->gui) return -1;

    CF_DIAGNOSTIC_POP()

    // Initialize OpenGL loader
    if (!gloadInit(NULL) || !gloadIsSupported((I32)gl_ver.major, (I32)gl_ver.minor))
    {
        logError("Failed to initialize OpenGL loader!\n");
        return -2;
    }
    CF_ASSERT_NOT_NULL(gl);
    platform->gl = gl;

    // Setup application
    AppApi app_api = {0};
    appApiLoad(&app_api, platform->paths, platform->library, platform->file);
    AppState *app_state = app_api.create(platform, cmd_line);

    // Setup Platform/Renderer backends
    guiGl3Init(gl_ver);

    // Main loop
    AppIo app_io = {
        .font_opts = &(GuiFontOptions){.rasterizer_multiply = 1.0f},
        // NOTE (Matteo): Ensure font rebuild before first frame
        .rebuild_fonts = true,
        // NOTE (Matteo): Ensure fast first update
        .continuous_update = true,
    };

    IVec2 display = {0};
    while (guiEventLoop(!app_io.continuous_update, app_io.fullscreen, &display) && !app_io.quit)
    {
        //------------------//
        // Event processing //
        //------------------//

        // TODO (Matteo): Maybe improve a bit?
        if (app_io.window_title_changed)
        {
            guiSetTitle(app_io.window_title);
        }

        // NOTE (Matteo): Auto reloading of application library
        appApiUpdate(&app_api, platform, app_state);

        // Rebuild font atlas if required
        if (app_io.rebuild_fonts)
        {
            app_io.rebuild_fonts = false;
            guiUpdateAtlas(guiFonts(), app_io.font_opts);
            // Re-upload font texture on the GPU
            guiGl3UpdateFontsTexture();
        }

        //--------------//
        // Frame update //
        //--------------//

        // Start the Dear ImGui frame
        guiGl3NewFrame();
        guiNewFrame();

        // NOTE (Matteo): Setup GL viewport and clear buffers BEFORE app update in order to allow
        // the application code to draw directly using OpenGL
        LinearColor clear_color = colorToLinearMultiplied(app_io.back_color);

        glViewport(0, 0, display.width, display.height);
        glClearColor(clear_color.r, clear_color.g, clear_color.b, clear_color.a);
        glClear(GL_COLOR_BUFFER_BIT);

        // Application frame update
        app_api.update(app_state, &app_io);

        //-----------//
        // Rendering //
        //-----------//

        GuiDrawData *draw_data = guiRender();
        guiGl3Render(draw_data);
        guiUpdateViewports(true);

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
    guiGl3Shutdown();
    guiShutdown(platform->gui);

    app_api.destroy(app_state);

    return 0;
}

//------------------------------------------------------------------------------
// Internal functions
//------------------------------------------------------------------------------

static APP_FN(appProcStub)
{
    CF_UNUSED(app);
}

static APP_CREATE_FN(appCreateStub)
{
    CF_UNUSED(plat);
    CF_UNUSED(cmd_line);
    return NULL;
}

static APP_UPDATE_FN(appUpdateStub)
{
    CF_UNUSED(state);
    io->quit = true;
}

void
appApiLoad(AppApi *api, Paths *paths, LibraryApi *library, IoFileApi *file)
{
    if (api->lib)
    {
        CF_ASSERT(api->create, "");
        library->unload(api->lib);
        memClearStruct(api);
    }

    strPrint(api->src_file, Paths_Size, "%.*s%.*s", //
             (I32)paths->base.len, paths->base.buf, //
             (I32)paths->lib_name.len, paths->lib_name.buf);
    strPrint(api->dst_file, Paths_Size, "%s.tmp", api->src_file);

    Str dst_file = strFromCstr(api->dst_file);
    if (file->copy(strFromCstr(api->src_file), dst_file, true))
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
    if (platform->file->propertiesP(strFromCstr(api->src_file)).last_write >
        platform->file->propertiesP(strFromCstr(api->dst_file)).last_write)
    {
        api->unload(app);
        appApiLoad(api, platform->paths, platform->library, platform->file);
        api->load(app);
    }
}

//------------------------------------------------------------------------------
