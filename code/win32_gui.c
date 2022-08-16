
// Interface between platform layer and hosted applicatiom
#include "app.h"

// Gui library
#include "gui/gui.h"
#include "gui/gui_backend_gl3.h"
#include "gui/win.h"

// Backend libraries
#include "gl/gl_api.h"

// Base Win32 code
#include "win32_platform.c"

//------------------------------------------------------------------------------
// Application API
//------------------------------------------------------------------------------

typedef struct AppApi
{
    void *lib;

    AppCreateProc create;
    AppProc destroy;

    AppProc load;
    AppProc unload;

    AppUpdateProc update;

    // TODO (Matteo): reduce redundancy
    Char8 src_file[Paths_Size];
    Char8 dst_file[Paths_Size];
} AppApi;

CF_INTERNAL APP_FN(appProcStub);
CF_INTERNAL APP_CREATE_FN(appCreateStub);
CF_INTERNAL APP_UPDATE_FN(appUpdateStub);

CF_INTERNAL void appApiLoad(AppApi *api, Paths *paths);
CF_INTERNAL void appApiUpdate(AppApi *api, Paths *paths, AppState *app);

//------------------------------------------------------------------------------
// Main entry point
//------------------------------------------------------------------------------

CF_INTERNAL Usize
win32GetCommandLineArgs(MemAllocator alloc, CommandLine *out)
{
    Cstr16 cmd_line = GetCommandLineW();

    I32 num_args = 0;
    Cstr16 *args16 = (Cstr16 *)CommandLineToArgvW(cmd_line, &num_args);

    if (num_args < 0)
    {
        win32HandleLastError();
        return 0;
    }

    out->len = (Usize)(num_args);

    Usize out_size = (Usize)(out->len) * sizeof(*out->arg) + CF_MB(1);

    out->arg = memAlloc(alloc, out_size);
    Cstr buf = (Char8 *)(out->arg + out->len);

    for (Usize i = 0; i < out->len; ++i)
    {
        out->arg[i] = buf;
        Str16 arg16 = str16FromCstr(args16[i]);
        Usize size = win32Utf16To8(arg16, NULL, 0);
        win32Utf16To8(arg16, (Char8 *)out->arg[i], size);
        buf += size + 1; // Ensure space for the null-terminator
    }

    LocalFree((Char16 *)args16);

    return out_size;
}

CF_INTERNAL void *
win32libLoad(Str filename)
{
    Char16 buffer[1024] = {0};
    win32Utf8To16(filename, buffer, CF_ARRAY_SIZE(buffer));
    void *lib = LoadLibraryW(buffer);

    if (!lib)
    {
        win32HandleLastError();
        CF_ASSERT(false, "LoadLibraryW FAILED");
    }

    return lib;
}

CF_INTERNAL void
win32libUnload(void *lib)
{
    if (!FreeLibrary((HMODULE)lib))
    {
        win32HandleLastError();
        CF_ASSERT(false, "FreeLibrary FAILED");
    }
}

CF_INTERNAL void *
win32libLoadProc(void *lib, Cstr name)
{
    return (void *)GetProcAddress((HMODULE)lib, name);
}

int WINAPI
wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR pCmdLine, int nCmdShow)
{
    CF_UNUSED(hInstance);
    CF_UNUSED(hPrevInstance);
    CF_UNUSED(pCmdLine);
    CF_UNUSED(nCmdShow);

    win32PlatformInit();

    // ** Platform-agnostic entry point **

    // TODO (Matteo): Improve command line handling

    CommandLine cmd_line = {0};
    Usize cmd_line_size = win32GetCommandLineArgs(g_platform.heap, &cmd_line);
    Paths *paths = g_platform.paths;

    // Setup platform layer
    clockStart(&g_platform.clock);

    // NOTE (Matteo): Custom IMGUI ini file
    // TODO (Matteo): Clean up!

    CF_DIAGNOSTIC_PUSH()
    CF_DIAGNOSTIC_IGNORE_MSVC(4221)

    Char8 gui_ini[Paths_Size] = {0};
    CF_ASSERT(paths->base.len + paths->exe_name.len < Paths_Size, "IMGUI ini file name too long");
    memCopy(paths->base.ptr, gui_ini, paths->base.len);
    memCopy(paths->exe_name.ptr, gui_ini + paths->base.len, paths->exe_name.len);
    pathChangeExt(strFromCstr(gui_ini), strLiteral(".gui"), gui_ini);

    // Setup Dear ImGui context
    GuiOpenGLVersion gl_ver = {0};
    g_platform.gui = guiInit(
        &(GuiInitInfo){
            .alloc = g_platform.heap,
            .ini_filename = gui_ini,
            .data_path = paths->data,
            .gl_context = true,
        },
        &gl_ver);

    if (!g_platform.gui) return -1;

    CF_DIAGNOSTIC_POP()

    // Initialize OpenGL loader
    if (!glApiLoad(guiLoadProc) || !glApiIsSupported(gl_ver.major, gl_ver.minor))
    {
        CF_LOG("Failed to initialize OpenGL loader!\n");
        return -2;
    }
    CF_ASSERT_NOT_NULL(gl);
    g_platform.gl = gl;

    // Setup application
    AppApi app_api = {0};
    appApiLoad(&app_api, g_platform.paths);
    AppState *app_state = app_api.create(&g_platform, &cmd_line);

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
        appApiUpdate(&app_api, paths, app_state);

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
    }

    // Cleanup
    guiGl3Shutdown();
    guiShutdown(g_platform.gui);
    app_api.destroy(app_state);

    memFree(g_platform.heap, (Char8 *)cmd_line.arg, cmd_line_size);

    win32PlatformShutdown();

    return 0;
}

//------------------------------//
//   Application API handling   //
//------------------------------//

APP_FN(appProcStub)
{
    CF_UNUSED(app);
}

APP_CREATE_FN(appCreateStub)
{
    CF_UNUSED(plat);
    CF_UNUSED(cmd_line);
    return NULL;
}

APP_UPDATE_FN(appUpdateStub)
{
    CF_UNUSED(state);
    io->quit = true;
}

void
appApiLoad(AppApi *api, Paths *paths)
{
    if (api->lib)
    {
        CF_ASSERT(api->create, "");
        win32libUnload(api->lib);
        memClearStruct(api);
    }

    strPrint(api->src_file, Paths_Size, "%.*s%.*s", //
             (I32)paths->base.len, paths->base.ptr, //
             (I32)paths->lib_name.len, paths->lib_name.ptr);
    strPrint(api->dst_file, Paths_Size, "%s.tmp", api->src_file);

    Str dst_file = strFromCstr(api->dst_file);
    if (win32FileCopy(strFromCstr(api->src_file), dst_file, true))
    {
        api->lib = win32libLoad(dst_file);
    }

    if (api->lib)
    {
        api->create = (AppCreateProc)win32libLoadProc(api->lib, "appCreate");
        api->destroy = (AppProc)win32libLoadProc(api->lib, "appDestroy");
        api->load = (AppProc)win32libLoadProc(api->lib, "appLoad");
        api->unload = (AppProc)win32libLoadProc(api->lib, "appUnload");
        api->update = (AppUpdateProc)win32libLoadProc(api->lib, "appUpdate");
    }

    if (!api->create) api->create = appCreateStub;
    if (!api->destroy) api->destroy = appProcStub;
    if (!api->load) api->load = appProcStub;
    if (!api->unload) api->unload = appProcStub;
    if (!api->update) api->update = appUpdateStub;
}

void
appApiUpdate(AppApi *api, Paths *paths, AppState *app)
{
    // TODO (Matteo): Are these operation too expensive to be performed every frame?
    if (win32FilePropertiesP(strFromCstr(api->src_file)).last_write >
        win32FilePropertiesP(strFromCstr(api->dst_file)).last_write)
    {
        api->unload(app);
        appApiLoad(api, paths);
        api->load(app);
    }
}
