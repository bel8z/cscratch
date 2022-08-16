
// Interface between platform layer and hosted applicatiom
#include "app.h"

#include "gl/gl_api.h"

#include "gui/gui.h"

// Base Win32 code
#include "win32_gui_backend.h"
#include "win32_platform.c"

/// Mantains the hot-reloaded application API
typedef struct Win32AppApi
{
    HMODULE lib;

    AppCreateProc create;
    AppProc destroy;

    AppProc load;
    AppProc unload;

    AppUpdateProc update;

    // TODO (Matteo): reduce redundancy
    Char8 src_file[Paths_Size];
    Char8 dst_file[Paths_Size];
} Win32AppApi;

//------------------------------------------------------------------------------
// Internal utilities
//------------------------------------------------------------------------------

CF_INTERNAL void win32GuiErrorHandlerV(Cstr format, va_list args, void *context) CF_VPRINTF_LIKE(0);

CF_GLOBAL Char16 g_buf[1024] = {0};

CF_INTERNAL Char16 const *
win32ConvertStr(Str str)
{
    Usize len = win32Utf8To16(str, g_buf, CF_ARRAY_SIZE(g_buf));

    CF_ASSERT(len < CF_ARRAY_SIZE(g_buf) - 1, "Overflow");
    g_buf[len] = 0;

    return g_buf;
}

CF_INTERNAL void
win32GuiErrorHandler(Str message)
{
    MessageBoxW(NULL, win32ConvertStr(message), L"", 0);
}

void
win32GuiErrorHandlerV(Cstr format, va_list args, void *context)
{
    CF_UNUSED(context);

    static Char8 buf[1024] = {0};
    Isize len = strPrintV(buf, CF_ARRAY_SIZE(buf), format, args);

    if (len > 0)
    {
        win32GuiErrorHandler((Str){.ptr = (Char8 const *)buf, .len = (Usize)len});
    }
}

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

//------------------------------------------------------------------------------
// WGL stuff
//------------------------------------------------------------------------------

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

// See https://www.khronos.org/registry/OpenGL/extensions/ARB/WGL_ARB_create_context.txt for all
// values
#define WGL_CONTEXT_MAJOR_VERSION_ARB 0x2091
#define WGL_CONTEXT_MINOR_VERSION_ARB 0x2092
#define WGL_CONTEXT_FLAGS_ARB 0x2094
#define WGL_CONTEXT_PROFILE_MASK_ARB 0x9126

#define WGL_CONTEXT_DEBUG_BIT_ARB 0x0001
#define WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB 0x0002

#define WGL_CONTEXT_CORE_PROFILE_BIT_ARB 0x00000001
#define WGL_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB 0x00000002

// See https://www.khronos.org/registry/OpenGL/extensions/ARB/WGL_ARB_pixel_format.txt for all
// values
#define WGL_DRAW_TO_WINDOW_ARB 0x2001
#define WGL_ACCELERATION_ARB 0x2003
#define WGL_SUPPORT_OPENGL_ARB 0x2010
#define WGL_DOUBLE_BUFFER_ARB 0x2011
#define WGL_PIXEL_TYPE_ARB 0x2013
#define WGL_COLOR_BITS_ARB 0x2014
#define WGL_DEPTH_BITS_ARB 0x2022
#define WGL_STENCIL_BITS_ARB 0x2023

#define WGL_FULL_ACCELERATION_ARB 0x2027
#define WGL_TYPE_RGBA_ARB 0x202B

typedef HGLRC WINAPI WglCreateContextAttribsARBFn(HDC hdc, HGLRC hShareContext,
                                                  I32 const *attribList);

typedef BOOL WINAPI WglChoosePixelFormatARBFn(HDC hdc, I32 const *piAttribIList,
                                              const F32 *pfAttribFList, U32 nMaxFormats,
                                              I32 *piFormats, U32 *nNumFormats);
typedef BOOL WglSwapIntervalEXTFn(I32 interval);

CF_GLOBAL WglCreateContextAttribsARBFn *wglCreateContextAttribsARB;
CF_GLOBAL WglChoosePixelFormatARBFn *wglChoosePixelFormatARB;
CF_GLOBAL WglSwapIntervalEXTFn *wglSwapIntervalEXT;

CF_INTERNAL void *
win32LoadGlProc(Cstr name)
{
    void *proc = (void *)wglGetProcAddress(name);
    if (proc)
    {
        return proc;
    }

    return (void *)GetProcAddress(GetModuleHandleW(L"opengl32"), name);
}

CF_INTERNAL bool
win32InitWgl(void)
{
    // Before we can load extensions, we need a dummy OpenGL context, created using a dummy window.
    // We use a dummy window because you can only set the pixel format for a window once. For the
    // real window, we want to use wglChoosePixelFormatARB (so we can potentially specify options
    // that aren't available in PIXELFORMATDESCRIPTOR), but we can't load and use that before we
    // have a context.
    WNDCLASSA window_class = {
        .style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC,
        .lpfnWndProc = DefWindowProcA,
        .hInstance = GetModuleHandle(0),
        .lpszClassName = "WGL_Boostrap_Window",
    };

    if (!RegisterClassA(&window_class))
    {
        win32GuiErrorHandler(strLiteral("Failed to register dummy OpenGL window."));
        return false;
    }

    HWND dummy_window = CreateWindowExA(0, window_class.lpszClassName, "Dummy OpenGL Window", 0,
                                        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
                                        0, 0, window_class.hInstance, 0);

    if (!dummy_window)
    {
        win32GuiErrorHandler(strLiteral("Failed to create dummy OpenGL window."));
        return false;
    }

    HDC dummy_dc = GetDC(dummy_window);

    PIXELFORMATDESCRIPTOR pfd = {
        .nSize = sizeof(pfd),
        .nVersion = 1,
        .iPixelType = PFD_TYPE_RGBA,
        .dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,
        .cColorBits = 32,
        .cAlphaBits = 8,
        .iLayerType = PFD_MAIN_PLANE,
        .cDepthBits = 24,
        .cStencilBits = 8,
    };

    I32 pixel_format = ChoosePixelFormat(dummy_dc, &pfd);
    if (!pixel_format)
    {
        win32GuiErrorHandler(strLiteral("Failed to find a suitable pixel format."));
        return false;
    }

    if (!SetPixelFormat(dummy_dc, pixel_format, &pfd))
    {
        win32GuiErrorHandler(strLiteral("Failed to set the pixel format."));
        return false;
    }

    HGLRC dummy_context = wglCreateContext(dummy_dc);
    if (!dummy_context)
    {
        win32GuiErrorHandler(strLiteral("Failed to create a dummy OpenGL rendering context."));
        return false;
    }

    if (!wglMakeCurrent(dummy_dc, dummy_context))
    {
        win32GuiErrorHandler(strLiteral("Failed to activate dummy OpenGL rendering context."));
        return false;
    }

    wglCreateContextAttribsARB =
        (WglCreateContextAttribsARBFn *)win32LoadGlProc("wglCreateContextAttribsARB");
    wglChoosePixelFormatARB =
        (WglChoosePixelFormatARBFn *)win32LoadGlProc("wglChoosePixelFormatARB");
    wglSwapIntervalEXT = (WglSwapIntervalEXTFn *)win32LoadGlProc("wglSwapIntervalEXT");

    wglMakeCurrent(dummy_dc, 0);
    wglDeleteContext(dummy_context);
    ReleaseDC(dummy_window, dummy_dc);
    DestroyWindow(dummy_window);

    return true;
}

CF_INTERNAL HGLRC
win32CreateGlContext(HDC dc, OpenGLVersion *out_ver)
{
    // clang-format off
    static I32 pixel_format_attribs[] = {
        WGL_DRAW_TO_WINDOW_ARB, GL_TRUE, 
        WGL_SUPPORT_OPENGL_ARB, GL_TRUE,
        WGL_DOUBLE_BUFFER_ARB,  GL_TRUE,
        WGL_ACCELERATION_ARB,   WGL_FULL_ACCELERATION_ARB,
        WGL_PIXEL_TYPE_ARB,     WGL_TYPE_RGBA_ARB,
        WGL_COLOR_BITS_ARB,     32,
        WGL_DEPTH_BITS_ARB,     24,
        WGL_STENCIL_BITS_ARB,   8,
        0,
    };
    // clang-format on

    I32 pixel_format;
    U32 num_formats;
    wglChoosePixelFormatARB(dc, pixel_format_attribs, 0, 1, &pixel_format, &num_formats);
    if (!num_formats)
    {
        win32GuiErrorHandler(strLiteral("Failed to set the OpenGL pixel format."));
        return NULL;
    }

    PIXELFORMATDESCRIPTOR pfd;
    DescribePixelFormat(dc, pixel_format, sizeof(pfd), &pfd);
    if (!SetPixelFormat(dc, pixel_format, &pfd))
    {
        win32GuiErrorHandler(strLiteral("Failed to set the OpenGL pixel format."));
        return NULL;
    }

    HGLRC gl_context = NULL;

    // NOTE (Matteo): I didn't find a way to ask WGL for a core profile context with the highest
    // version available, so the best I can do is trying some different versions in decreasing
    // release order (one per year of release).
    static const OpenGLVersion gl_versions[] = {
        {4, 6, 430}, // 2017
        {4, 5, 430}, // 2014
        {4, 4, 430}, // 2013
        {4, 3, 430}, // 2012
        {4, 2, 420}, // 2011
        {4, 1, 410}, // 2010
        {3, 2, 150}, // 2009
    };

    // Specify that we want to create an OpenGL core profile context
    // clang-format off
    I32 context_attribs[] = {
        WGL_CONTEXT_MAJOR_VERSION_ARB, 0, // PLACEHOLDER
        WGL_CONTEXT_MINOR_VERSION_ARB, 0, // PLACEHOLDER
        WGL_CONTEXT_FLAGS_ARB,         WGL_CONTEXT_DEBUG_BIT_ARB | WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB,
        WGL_CONTEXT_PROFILE_MASK_ARB,  WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
        0,
    };
    // clang-format on

    for (Usize i = 0; i < CF_ARRAY_SIZE(gl_versions); ++i)
    {
        context_attribs[1] = (I32)gl_versions[i].major;
        context_attribs[3] = (I32)gl_versions[i].minor;

        CF_ASSERT(context_attribs[1] >= 3 && context_attribs[3] >= 2,
                  "OpenGL 3.2 or major is expected");

        gl_context = wglCreateContextAttribsARB(dc, 0, context_attribs);
        if (gl_context)
        {
            *out_ver = gl_versions[i];
            break;
        }
    }

    if (!gl_context)
    {
        win32GuiErrorHandler(strLiteral("Failed to create OpenGL context."));
        return NULL;
    }

    if (!wglMakeCurrent(dc, gl_context))
    {
        win32GuiErrorHandler(strLiteral("Failed to activate OpenGL rendering context."));
        return NULL;
    }

    return gl_context;
}

#if 0

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

#endif

//------------------------------//
//   Application API handling   //
//------------------------------//

CF_INTERNAL APP_FN(win32AppProc)
{
    CF_UNUSED(app);
}

CF_INTERNAL APP_CREATE_FN(win32AppCreate)
{
    CF_UNUSED(plat);
    CF_UNUSED(cmd_line);
    return NULL;
}

CF_INTERNAL APP_UPDATE_FN(win32AppUpdate)
{
    CF_UNUSED(state);
    io->quit = true;
}

CF_INTERNAL void
win32LoadAppApi(Win32AppApi *api, Paths *paths)
{
    if (api->lib)
    {
        CF_ASSERT(api->create, "");
        if (!FreeLibrary(api->lib)) win32HandleLastError();
        memClearStruct(api);
    }

    strPrint(api->src_file, Paths_Size, "%.*s%.*s", //
             (I32)paths->base.len, paths->base.ptr, //
             (I32)paths->lib_name.len, paths->lib_name.ptr);
    strPrint(api->dst_file, Paths_Size, "%s.tmp", api->src_file);

    Str dst_file = strFromCstr(api->dst_file);
    if (win32FileCopy(strFromCstr(api->src_file), dst_file, true))
    {
        api->lib = LoadLibraryW(win32ConvertStr(dst_file));
    }

    if (api->lib)
    {
        api->create = (AppCreateProc)GetProcAddress(api->lib, "appCreate");
        api->destroy = (AppProc)GetProcAddress(api->lib, "appDestroy");
        api->load = (AppProc)GetProcAddress(api->lib, "appLoad");
        api->unload = (AppProc)GetProcAddress(api->lib, "appUnload");
        api->update = (AppUpdateProc)GetProcAddress(api->lib, "appUpdate");
    }
    else
    {
        win32HandleLastError();
    }

    if (!api->create) api->create = win32AppCreate;
    if (!api->destroy) api->destroy = win32AppProc;
    if (!api->load) api->load = win32AppProc;
    if (!api->unload) api->unload = win32AppProc;
    if (!api->update) api->update = win32AppUpdate;
}

CF_INTERNAL void
win32UpdateAppApi(Win32AppApi *api, Paths *paths, AppState *app)
{
    // TODO (Matteo): Are these operation too expensive to be performed every frame?
    if (win32FilePropertiesP(strFromCstr(api->src_file)).last_write >
        win32FilePropertiesP(strFromCstr(api->dst_file)).last_write)
    {
        api->unload(app);
        win32LoadAppApi(api, paths);
        api->load(app);
    }
}

//------------------------------------------------------------------------------
// Main entry point
//------------------------------------------------------------------------------

CF_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

LRESULT WINAPI
WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (g_platform.gui && ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam)) return true;

    switch (msg)
    {
        case WM_SIZE: return 0;
        case WM_SYSCOMMAND:
            if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
                return 0;
            break;
        case WM_DESTROY:
            // NOTE (Matteo): Shutdown is done here because the GL API is dependent on the current
            // context, which is going to be destroyed.
            win32GuiShutdown();
            wglDeleteContext(wglGetCurrentContext());
            PostQuitMessage(0);
            return 0;
        case WM_DPICHANGED:
#if 0
            if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_DpiEnableScaleViewports)
            {
                // const int dpi = HIWORD(wParam);
                // printf("WM_DPICHANGED to %d (%.0f%%)\n", dpi, (float)dpi / 96.0f * 100.0f);
                const RECT *suggested_rect = (RECT *)lParam;
                ::SetWindowPos(hWnd, NULL, suggested_rect->left, suggested_rect->top,
                               suggested_rect->right - suggested_rect->left,
                               suggested_rect->bottom - suggested_rect->top,
                               SWP_NOZORDER | SWP_NOACTIVATE);
            }
#endif
            break;
    }

    return DefWindowProcW(hWnd, msg, wParam, lParam);
}

I32 WINAPI
wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR pCmdLine, I32 nCmdShow)
{
    I32 result = -1;

    CF_UNUSED(hInstance);
    CF_UNUSED(hPrevInstance);
    CF_UNUSED(pCmdLine);
    CF_UNUSED(nCmdShow);

    // Setup platform layer
    win32PlatformInit();
    // TODO (Matteo): Improve command line handling
    CommandLine cmd_line = {0};
    Usize cmd_line_size = win32GetCommandLineArgs(g_platform.heap, &cmd_line);
    Paths *paths = g_platform.paths;
    clockStart(&g_platform.clock);

    // Setup windowing system
    SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

    WNDCLASSEXW wc = {
        .cbSize = sizeof(WNDCLASSEXW),
        .style = CS_CLASSDC,
        .lpfnWndProc = WndProc,
        .hInstance = GetModuleHandle(NULL),
        .lpszClassName = L"CSCRATCH",
    };
    RegisterClassExW(&wc);

    // Setup error handling
    errorInstallHandler(win32GuiErrorHandlerV, NULL);

    // NOTE (Matteo): Custom IMGUI ini file
    // TODO (Matteo): Clean up!
    Char8 gui_ini[Paths_Size] = {0};
    {
        CF_DIAGNOSTIC_PUSH()
        CF_DIAGNOSTIC_IGNORE_MSVC(4221)

        CF_ASSERT(paths->base.len + paths->exe_name.len < Paths_Size,
                  "IMGUI ini file name too long");
        memCopy(paths->base.ptr, gui_ini, paths->base.len);
        memCopy(paths->exe_name.ptr, gui_ini + paths->base.len, paths->exe_name.len);
        pathChangeExt(strFromCstr(gui_ini), strLiteral(".gui"), gui_ini);

        CF_DIAGNOSTIC_POP()
    }

    // Create application window with graphics context
    OpenGLVersion gl_ver;
    HWND window = NULL;
    HDC gl_dc = NULL;
    HGLRC gl_context = NULL;
    {
        if (!win32InitWgl()) goto WIN32_PLATORM_SHUTDOWN;

        window =
            CreateWindowW(wc.lpszClassName, win32ConvertStr(strLiteral("Dear ImGui template")),
                          WS_OVERLAPPEDWINDOW, 100, 100, 1280, 720, NULL, NULL, wc.hInstance, NULL);
        if (!window) goto WIN32_PLATORM_SHUTDOWN;

        // Show the window
        ShowWindow(window, SW_SHOWDEFAULT);
        UpdateWindow(window);

        gl_dc = GetDC(window);
        gl_context = win32CreateGlContext(gl_dc, &gl_ver);
        if (!gl_context) goto WIN32_PLATORM_SHUTDOWN;

        wglMakeCurrent(gl_dc, gl_context);
        wglSwapIntervalEXT(1); // Enable vsync
    }

    // Initialize OpenGL loader
    if (!glApiLoad((GlApiLoadFn *)(&win32LoadGlProc)) ||
        !glApiIsSupported(gl_ver.major, gl_ver.minor))
    {
        CF_LOG("Failed to initialize OpenGL loader!\n");
        goto WIN32_PLATORM_SHUTDOWN;
    }

    CF_ASSERT_NOT_NULL(gl);
    g_platform.gl = gl;

    // Setup Dear ImGui context
    {
        // HACK How do I get the platform base DPI?
        F32 dpi_scale = win32GuiGetDpiScale(window);

        g_platform.gui = guiInit(
            &(GuiInitInfo){
                .alloc = g_platform.heap,
                .ini_filename = gui_ini,
                .data_path = paths->data,
            },
            dpi_scale);

        if (!g_platform.gui) goto WIN32_PLATORM_SHUTDOWN;

        // Setup Platform/Renderer backends
        win32GuiInit(window, gl_ver);
    }

    // Setup application
    Win32AppApi app_api = {0};
    win32LoadAppApi(&app_api, g_platform.paths);
    AppState *app_state = app_api.create(&g_platform, &cmd_line);

    // Main loop
    AppIo app_io = {
        .font_opts = &(GuiFontOptions){.rasterizer_multiply = 1.0f},
        // NOTE (Matteo): Ensure font rebuild before first frame
        .rebuild_fonts = true,
        // NOTE (Matteo): Ensure fast first update
        .continuous_update = true,
    };

    while (true)
    {
        // Ensure presentation on every frame
        wglMakeCurrent(gl_dc, gl_context);
        glFlush();
        SwapBuffers(gl_dc);

        // Process events
        {
            if (app_io.quit) DestroyWindow(window);
            if (!app_io.continuous_update) WaitMessage();

            MSG msg = {0};
            bool quit = false;

            while (PeekMessageW(&msg, 0, 0, 0, PM_REMOVE))
            {
                if (msg.message == WM_QUIT)
                {
                    quit = true;
                }
                else
                {
                    TranslateMessage(&msg);
                    DispatchMessageW(&msg);
                }
            }

            if (quit) break;
        }

        // NOTE (Matteo): Auto reloading of application library
        win32UpdateAppApi(&app_api, paths, app_state);

        // Process application requests
        {
            // TODO (Matteo): Handle fullscreen

            // TODO (Matteo): Maybe improve a bit?
            if (app_io.window_title_changed)
            {
                SetWindowTextW(window, win32ConvertStr(app_io.window_title));
            }

            // Rebuild font atlas if required
            if (app_io.rebuild_fonts)
            {
                app_io.rebuild_fonts = false;
                guiUpdateAtlas(guiFonts(), app_io.font_opts);
                // Re-upload font texture on the GPU
                win32GuiUpdateFontsTexture();
            }
        }

        // Frame update
        {
            // Start the Dear ImGui frame
            win32GuiNewFrame();
            guiNewFrame();

            // NOTE (Matteo): Setup GL viewport and clear buffers BEFORE app update in order to
            // allow the application code to draw directly using OpenGL
            LinearColor clear_color = colorToLinearMultiplied(app_io.back_color);

            RECT rect;
            GetClientRect(window, &rect);
            IVec2 display = {
                .x = rect.right - rect.left,
                .y = rect.bottom - rect.top,
            };
            glViewport(0, 0, display.x, display.y);
            glClearColor(clear_color.r, clear_color.g, clear_color.b, clear_color.a);
            glClear(GL_COLOR_BUFFER_BIT);

            // Application frame update
            app_api.update(app_state, &app_io);
        }

        // Rendering
        {
            GuiDrawData *draw_data = guiRender();
            win32GuiRender(draw_data);

            // Update and Render additional Platform Windows
            // TODO (Matteo): Make sure the GL context is restored? Currently it
            // is done at every iteration
            guiUpdateViewports(true);
        }
    }

    // Cleanup
    guiShutdown(g_platform.gui);
    app_api.destroy(app_state);

    // NOTE (Matteo): Setting success only before shutdown, in case other errors required exit.
    result = 0;

WIN32_PLATORM_SHUTDOWN:
    memFree(g_platform.heap, (Char8 *)cmd_line.arg, cmd_line_size);
    win32PlatformShutdown();

    return result;
}
