// Interface between platform layer and hosted applicatiom
#include "api.h"

// Gui library
#include "gui.h"

// Backend libraries
#include "gl/gload.h"

#if SDL_BACKEND
#define SDL_MAIN_HANDLED
#include <SDL.h>
#else
#include <GLFW/glfw3.h>
#endif

// Foundation library
#include "foundation/allocator.h"
#include "foundation/color.h"
#include "foundation/common.h"
#include "foundation/path.h"
#include "foundation/strings.h"
#include "foundation/util.h"
#include "foundation/vec.h"

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
glfw_error_callback(int error, const char *description)
{
    fprintf(stderr, "Glfw Error %d: %s\n", error, description);
}

#endif

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
#ifdef __APPLE__
    // GL 3.2 Core + GLSL 150
    const char *glsl_version = "#version 150";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS,
                        SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG); // Always required on Mac
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
#else
    // GL 3.0 + GLSL 130
    const char *glsl_version = "#version 130";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#endif

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
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit()) return 1;

        // Decide GL+GLSL versions
#ifdef __APPLE__
    // GL 3.2 + GLSL 150
    const char *glsl_version = "#version 150";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE); // 3.2+ only
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);           // Required on Mac
#else
    // GL 3.0 + GLSL 130
    const char *glsl_version = "#version 130";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    // glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
    // glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // 3.0+ only
#endif

    // Create window with graphics context
    GLFWwindow *window = glfwCreateWindow(1280, 720, "Dear ImGui template", NULL, NULL);
    if (window == NULL) return 1;
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Enable vsync
#endif

    // Initialize OpenGL loader
    if (!gloadInit(NULL))
    {
        fprintf(stderr, "Failed to initialize OpenGL loader!\n");
        return -2;
    }

    // Setup platform layer
    CF_ASSERT_NOT_NULL(gl);
    platform->gl = gl;

    // Setup application
    AppState *app = appCreate(platform, argv, argc);

    // Setup Dear ImGui context
    igDebugCheckVersionAndDataLayout("1.82", sizeof(ImGuiIO), sizeof(ImGuiStyle), sizeof(ImVec2),
                                     sizeof(ImVec4), sizeof(ImDrawVert), sizeof(ImDrawIdx));
    igSetAllocatorFunctions(guiAlloc, guiFree, platform->heap);
    ImGuiContext *imgui = igCreateContext(NULL);
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
    ImGui_ImplOpenGL3_Init(glsl_version);

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
        update_result = appUpdate(app, &font_opts);

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
    igDestroyContext(imgui);

#if SDL_BACKEND
    SDL_GL_DeleteContext(gl_context);
    SDL_DestroyWindow(window);
    SDL_Quit();
#else
    glfwDestroyWindow(window);
    glfwTerminate();
#endif

    appDestroy(app);

    return 0;
}

//------------------------------------------------------------------------------
// Internal functions
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
