#include "foundation/allocator.h"
#include "foundation/common.h"
#include "foundation/maths.h"
#include "foundation/util.h"

#include "imgui_decl.h"

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wsign-conversion"
#pragma clang diagnostic ignored "-Wsign-compare"
#pragma clang diagnostic ignored "-Wimplicit-int-conversion"
#endif

#define STB_IMAGE_IMPLEMENTATION
#include "ext/stb/stb_image.h"

#if defined(__clang__)
#pragma clang diagnostic pop
#endif

#include <GL/gl3w.h> // Initialize with gl3wInit()

#define SDL_BACKEND 0

#if SDL_BACKEND
#define SDL_MAIN_HANDLED
#include <SDL.h>
#else
#include <GLFW/glfw3.h>
#endif

#include <stdio.h>
#include <stdlib.h>

//------------------------------------------------------------------------------
// Local data & functions
//------------------------------------------------------------------------------

typedef struct FontOptions
{
    f32 rasterizer_multiply;
    // Freetype only
    bool freetype_enabled;
    u32 freetype_flags;
    // Stb only
    i32 oversample_h;
    i32 oversample_v;
} FontOptions;

typedef struct Image
{
    GLuint texture;
    ImVec2 size;
} Image;

typedef struct AppPaths
{
    char base[1024];
    char data[1024];
} AppPaths;

typedef struct AppWindows
{
    bool demo;
    bool metrics;
    bool stats;
    bool fonts;
    bool style;

} AppWindows;

typedef struct AppState
{
    cfAllocator *alloc;

    AppPaths paths;

    FontOptions font_opts;
    bool rebuild_fonts;

    AppWindows windows;
    ImVec4 clear_color;

    Image image;
    f32 zoom;

} AppState;

static CF_ALLOCATOR_FUNC(appRealloc);
static CF_ALLOC_STATS_FUNC(appAllocStats);
static void appInitPaths(AppPaths *paths);

static void *guiAlloc(usize size, void *state);
static void guiFree(void *mem, void *state);
static bool guiBeforeUpdate(AppState *state);
static void guiUpdate(AppState *state, f32 framerate);
static void guiSetupStyle(f32 dpi);
static void guiSetupFonts(ImFontAtlas *fonts, f32 dpi, char const *data_path);

// Simple helper function to load an image into a OpenGL texture with common settings
static bool imageLoadFromFile(Image *image, const char *filename);

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
extern void ImGui_ImplGlfw_MouseButtonCallback(GLFWwindow *window, int button, int action,
                                               int mods);
extern void ImGui_ImplGlfw_ScrollCallback(GLFWwindow *window, double xoffset, double yoffset);
extern void ImGui_ImplGlfw_KeyCallback(GLFWwindow *window, int key, int scancode, int action,
                                       int mods);
extern void ImGui_ImplGlfw_CharCallback(GLFWwindow *window, unsigned int c);
extern void ImGui_ImplGlfw_MonitorCallback(GLFWmonitor *monitor, int event);

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

int
main(int argc, char **argv)
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
    if (gl3wInit() != 0)
    {
        fprintf(stderr, "Failed to initialize OpenGL loader!\n");
        return -2;
    }

    // Setup memory management
    // TODO (Matteo): make allocation not dependent to SDL
    cfAllocatorStats alloc_stats = {0};
    cfAllocator alloc = {.state = &alloc_stats, .reallocate = appRealloc, .stats = appAllocStats};
    igSetAllocatorFunctions(guiAlloc, guiFree, &alloc);

    // Setup application state
    AppState state = {
        .alloc = &alloc,
        .rebuild_fonts = true,
        .font_opts = {.freetype_enabled = true,
                      .freetype_flags = 0,
                      .oversample_h = 1,
                      .oversample_v = 1,
                      .rasterizer_multiply = 1.0f},
        .windows =
            {
                .demo = false,
                .fonts = false,
                .metrics = false,
                .stats = false,
                .style = false,
            },
        .clear_color = {0.45f, 0.55f, 0.60f, 1.00f},
        .zoom = 1.0,
    };

    appInitPaths(&state.paths);

    char buffer[1024];
    snprintf(buffer, 1024, "%sOpaque.png", state.paths.data);
    imageLoadFromFile(&state.image, buffer);

    // Setup Dear ImGui context
    // IMGUI_CHECKVERSION();
    ImGuiContext *imgui = igCreateContext(NULL);
    ImGuiIO *io = igGetIO();

    io->ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
    io->ConfigFlags |= ImGuiConfigFlags_DockingEnable;     // Enable Docking
    io->ConfigFlags |= ImGuiConfigFlags_ViewportsEnable; // Enable Multi-Viewport / Platform Windows

    // Setup DPI handling
#if SDL_BACKEND
    f32 ddpi, hdpi, vdpi;
    if (SDL_GetDisplayDPI(0, &ddpi, &hdpi, &vdpi) != 0)
    {
        fprintf(stderr, "Failed to obtain DPI information for display 0: %s\n", SDL_GetError());
        return -3;
    }
#else
    f32 win_x_scale, win_y_scale;
    glfwGetWindowContentScale(window, &win_x_scale, &win_y_scale);
    // HACK How do I get the platform base DPI?
    f32 ddpi = 96.0f * win_x_scale;
#endif

    // Setup Dear ImGui style
    guiSetupStyle(ddpi);
    guiSetupFonts(io->Fonts, ddpi, state.paths.data);

// Setup Platform/Renderer backends
#if SDL_BACKEND
    ImGui_ImplSDL2_InitForOpenGL(window, gl_context);
#else
    ImGui_ImplGlfw_InitForOpenGL(window, true);
#endif
    ImGui_ImplOpenGL3_Init(glsl_version);

    // Main loop
    bool done = false;
    while (!done)
    {
        // Poll and handle events (inputs, window resize, etc.)
        // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear
        // imgui wants to use your inputs.
        // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main
        // application.
        // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your
        // main application. Generally you may always pass all inputs to dear imgui, and hide
        // them from your application based on those two flags.

#if SDL_BACKEND
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
        done = glfwWindowShouldClose(window);
        glfwPollEvents();
#endif

        if (guiBeforeUpdate(&state))
        {
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
        guiUpdate(&state, io->Framerate);

        // Rendering
        igRender();

#if SDL_BACKEND
        glViewport(0, 0, (i32)io->DisplaySize.x, (i32)io->DisplaySize.y);
#else
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);

#endif

        glClearColor(state.clear_color.x * state.clear_color.w,
                     state.clear_color.y * state.clear_color.w,
                     state.clear_color.z * state.clear_color.w, state.clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(igGetDrawData());

        // Update and Render additional Platform Windows
        // (Platform functions may change the current OpenGL context, so we save/restore it to
        // make it easier to paste this code elsewhere.)
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

    return 0;
}

//------------------------------------------------------------------------------
// Local functions
//------------------------------------------------------------------------------

CF_ALLOCATOR_FUNC(appRealloc)
{
    cfAllocatorStats *stats = state;

    if (new_size)
    {
        if (memory && old_size)
        {
            stats->size += new_size - old_size;
        }
        else
        {
            stats->count++;
            stats->size += new_size;
        }

        return realloc(memory, new_size);
    }

    if (memory)
    {
        stats->count -= 1;
        stats->size -= old_size;
        free(memory);
    }

    return NULL;
}

CF_ALLOC_STATS_FUNC(appAllocStats)
{
    cfAllocatorStats *stats = state;
    return *stats;
}

void
appInitPaths(AppPaths *paths)
{
    char *p = "./"; // SDL_GetBasePath();

    snprintf(paths->base, 1024, "%s", p);
    snprintf(paths->data, 1024, "%sdata/", p);

    // SDL_free(p);
}

//------------------------------------------------------------------------------
// Local functions
//------------------------------------------------------------------------------

void *
guiAlloc(usize size, void *state)
{
    cfAllocator *alloc = state;
    usize *mem = CF_ALLOCATE(alloc, size + sizeof(usize));
    if (mem)
    {
        *mem = size;
        ++mem;
    }
    return mem;
}

void
guiFree(void *mem, void *state)
{
    if (mem)
    {
        cfAllocator *alloc = state;
        usize *buf = mem;
        buf--;
        CF_DEALLOCATE(alloc, buf, *buf);
    }
}

bool
guiBeforeUpdate(AppState *state)
{
    if (!state->rebuild_fonts) return false;

    ImGuiIO *io = igGetIO();
    ImFontAtlas *fonts = io->Fonts;
    FontOptions *font_opts = &state->font_opts;

    for (i32 i = 0; i < fonts->ConfigData.Size; ++i)
    {
        fonts->ConfigData.Data[i].RasterizerMultiply = font_opts->rasterizer_multiply;
        fonts->ConfigData.Data[i].OversampleH = font_opts->oversample_h;
        fonts->ConfigData.Data[i].OversampleV = font_opts->oversample_v;
    }

    if (font_opts->freetype_enabled)
    {
        fonts->FontBuilderIO = ImGuiFreeType_GetBuilderForFreeType();
        fonts->FontBuilderFlags = (u32)font_opts->freetype_flags;
    }
    else
    {
        fonts->FontBuilderIO = igImFontAtlasGetBuilderForStbTruetype();
    }

    ImFontAtlas_Build(fonts);
    state->rebuild_fonts = false;

    return true;
}

static void
guiShowAllocStats(cfAllocatorStats const *stats, bool *p_open)
{
    igBegin("Allocation stats", p_open, 0);
    igLabelText("# of allocations", "%zu", stats->count);
    igLabelText("Size of allocations", "%zu", stats->size);
    igEnd();
}

static bool
guiShowFontOptions(FontOptions *state, bool *p_open)
{
    ImFontAtlas *atlas = igGetIO()->Fonts;
    bool rebuild_fonts = false;

    igBegin("Font Options", p_open, 0);
    igShowFontSelector("Fonts");

    if (igRadioButtonBool("FreeType", state->freetype_enabled))
    {
        state->freetype_enabled = true;
        rebuild_fonts = true;
    }
    guiSameLine();
    if (igRadioButtonBool("Stb (Default)", !state->freetype_enabled))
    {
        state->freetype_enabled = false;
        rebuild_fonts = true;
    }

    rebuild_fonts |= igDragInt("TexGlyphPadding", &atlas->TexGlyphPadding, 0.1f, 1, 16, NULL,
                               ImGuiSliderFlags_None);

    rebuild_fonts |= igDragFloat("RasterizerMultiply", &state->rasterizer_multiply, 0.001f, 0.0f,
                                 2.0f, NULL, ImGuiSliderFlags_None);

    igSeparator();

    if (state->freetype_enabled)
    {

        rebuild_fonts |= igCheckboxFlagsUintPtr("NoHinting", &state->freetype_flags,
                                                ImGuiFreeTypeBuilderFlags_NoHinting);
        rebuild_fonts |= igCheckboxFlagsUintPtr("NoAutoHint", &state->freetype_flags,
                                                ImGuiFreeTypeBuilderFlags_NoAutoHint);
        rebuild_fonts |= igCheckboxFlagsUintPtr("ForceAutoHint", &state->freetype_flags,
                                                ImGuiFreeTypeBuilderFlags_ForceAutoHint);
        rebuild_fonts |= igCheckboxFlagsUintPtr("LightHinting", &state->freetype_flags,
                                                ImGuiFreeTypeBuilderFlags_LightHinting);
        rebuild_fonts |= igCheckboxFlagsUintPtr("MonoHinting", &state->freetype_flags,
                                                ImGuiFreeTypeBuilderFlags_MonoHinting);
        rebuild_fonts |=
            igCheckboxFlagsUintPtr("Bold", &state->freetype_flags, ImGuiFreeTypeBuilderFlags_Bold);
        rebuild_fonts |= igCheckboxFlagsUintPtr("Oblique", &state->freetype_flags,
                                                ImGuiFreeTypeBuilderFlags_Oblique);
        rebuild_fonts |= igCheckboxFlagsUintPtr("Monochrome", &state->freetype_flags,
                                                ImGuiFreeTypeBuilderFlags_Monochrome);
    }
    else
    {
        rebuild_fonts |= igDragInt("Oversample H", &state->oversample_h, 0.1f, 1, 5, NULL,
                                   ImGuiSliderFlags_None);
        rebuild_fonts |= igDragInt("Oversample V", &state->oversample_v, 0.1f, 1, 5, NULL,
                                   ImGuiSliderFlags_None);
    }

    igEnd();

    return rebuild_fonts;
}

static void
guiImageView(Image const *image, f32 *zoom)
{
    f32 const min_zoom = 1.0f;
    f32 const max_zoom = 10.0f;

    ImVec4 const tint_color = {1, 1, 1, 1};
    ImVec4 const border_color = {1, 1, 1, 1};

    // NOTE (Matteo): in case of more precision required
    // i32 v = (i32)(1000 / *zoom);
    // i32 d = (1000 - v) / 2;
    // f32 uv = (f32)d * 0.001f;

    f32 uv = 0.5f * (1 - 1 / *zoom);
    ImVec2 uv0 = {uv, uv};
    ImVec2 uv1 = {1 - uv, 1 - uv};

    igImage((void *)(iptr)image->texture, image->size, uv0, uv1, tint_color, border_color);

    ImGuiIO *io = igGetIO();

    if (igIsItemHovered(0) && io->KeyCtrl)
    {
        *zoom = cfClamp(*zoom + io->MouseWheel, min_zoom, max_zoom);
    }

    igSliderFloat("zoom", zoom, min_zoom, max_zoom, "%.3f", 0);
}

void
guiUpdate(AppState *state, f32 framerate)
{
    // 1. Show the big demo window (Most of the sample code is in igShowDemoWindow()! You
    // can browse its code to learn more about Dear ImGui!).
    if (state->windows.demo) igShowDemoWindow(&state->windows.demo);

    // 2. Show a simple window that we create ourselves. We use a Begin/End pair to created
    // a named window.
    {
        static f32 f = 0.0f;
        static i32 counter = 0;

        // Create a window called "Hello, world!" and append into it.
        igBegin("Test window", NULL, 0);

        // Display some text (you can use a format strings  too)
        igText("This is some useful text.");

        // Edit bools storing our window open/close state
        igCheckbox("Demo Window", &state->windows.demo);

        // Edit 1 float using a slider from 0.0f to 1.0f
        igSliderFloat("float", &f, 0.0f, 1.0f, "%.3f", 0);
        // Edit 3 floats representing a color
        igColorEdit3("clear color", (float *)&state->clear_color, 0);

        // Buttons return true when clicked (most widgets return true
        // when edited/activated)
        if (guiButton("Button")) counter++;

        guiSameLine();
        igText("counter = %d", counter);

        igText("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / framerate, framerate);

        guiImageView(&state->image, &state->zoom);

        igEnd();
    }

    if (state->windows.fonts)
    {
        state->rebuild_fonts = guiShowFontOptions(&state->font_opts, &state->windows.fonts);
    }

    if (state->windows.stats)
    {
        cfAllocatorStats stats = CF_ALLOC_STATS(state->alloc);
        guiShowAllocStats(&stats, &state->windows.stats);
    }

    if (state->windows.style)
    {
        igBegin("Style Editor", &state->windows.style, 0);
        igShowStyleEditor(igGetStyle());
        igEnd();
    }

    if (state->windows.metrics)
    {
        igShowMetricsWindow(&state->windows.metrics);
    }

    if (igBeginMainMenuBar())
    {
        if (igBeginMenu("File", true)) igEndMenu();

        if (igBeginMenu("Windows", true))
        {
            igMenuItemBoolPtr("Style editor", NULL, &state->windows.style, true);
            igMenuItemBoolPtr("Font options", NULL, &state->windows.fonts, true);
            igSeparator();
            igMenuItemBoolPtr("Stats", NULL, &state->windows.stats, true);
            igMenuItemBoolPtr("Metrics", NULL, &state->windows.metrics, true);
            igSeparator();
            igMenuItemBoolPtr("Demo window", NULL, &state->windows.demo, true);
            igEndMenu();
        }

        igEndMainMenuBar();
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
guiSetupStyle(f32 dpi)
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

    ImGuiStyle_ScaleAllSizes(style, dpi / 96.0f);
}

static f32
guiScaleFontSize(f32 size, f32 dpi)
{
    return round(size * dpi / 72.0f);
}

ImFont *
guiLoadFont(ImFontAtlas *fonts, char const *data_path, char const *name, f32 font_size,
            ImWchar const *ranges)
{
    char buffer[1024] = {0};

    snprintf(buffer, 1024, "%s%s.ttf", data_path, name);

    return ImFontAtlas_AddFontFromFileTTF(fonts, buffer, font_size, NULL, ranges);
}

void
guiSetupFonts(ImFontAtlas *fonts, f32 dpi, char const *data_path)
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

    if (!guiLoadFont(fonts, data_path, "NotoSans", guiScaleFontSize(13, dpi), ranges) &
        !guiLoadFont(fonts, data_path, "OpenSans", guiScaleFontSize(13.5, dpi), ranges) &
        !guiLoadFont(fonts, data_path, "SourceSansPro", guiScaleFontSize(13.5, dpi), ranges))
    {
        ImFontAtlas_AddFontDefault(fonts, NULL);
    }
}

bool
imageLoadFromFile(Image *image, const char *filename)
{
    // Load from file
    i32 width = 0;
    i32 height = 0;
    u8 *data = stbi_load(filename, &width, &height, NULL, 4);
    if (!data) return false;

    // Create a OpenGL texture identifier
    GLuint image_texture;
    glGenTextures(1, &image_texture);
    glBindTexture(GL_TEXTURE_2D, image_texture);

    // Setup filtering parameters for display
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    // These are required on WebGL for non power-of-two textures
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // Upload pixels into texture
#if defined(GL_UNPACK_ROW_LENGTH) && !defined(__EMSCRIPTEN__)
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
#endif
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    stbi_image_free(data);

    image->texture = image_texture;
    image->size.x = (f32)width;
    image->size.y = (f32)height;

    return true;
}
