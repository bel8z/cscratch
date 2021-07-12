#include "api.h"

#include "foundation/colors.h"
#include "foundation/maths.h"
#include "foundation/strings.h"

#include "gl/gload.h"

#include "gui/gui.h"

typedef struct GfxState
{
    U32 VBO, VAO, shader;
} GfxState;

typedef struct AppWindows
{
    bool metrics;
    bool stats;
    bool fonts;
    bool style;
} AppWindows;

struct AppState
{
    Platform *plat;

    GfxState gfx;

    AppWindows windows;
    Rgba32 clear_color;
};

//------------------------------------------------------------------------------

#define log(...) fprintf(stderr, __VA_ARGS__)

static void gfxInit(GfxState *state);

APP_API APP_CREATE_PROC(appCreate)
{
    CF_UNUSED(argv);
    CF_UNUSED(argc);

    // NOTE (Matteo): Memory comes cleared to 0
    AppState *app = cfAlloc(plat->heap, sizeof(*app));

    app->plat = plat;
    app->clear_color = RGBA32_SOLID(115, 140, 153); // R = 0.45, G = 0.55, B = 0.60

    // Init Dear Imgui
    guiInit(plat->gui);

    // Init OpenGl
    gloadInit(plat->gl);
    if (!gloadIsSupported(3, 3))
    {
        // TODO (Matteo): Log using IMGUI
        log("OpenGL 3.3 is not supported!");
    }
    gfxInit(&app->gfx);

    return app;
}

APP_API APP_PROC(appDestroy)
{
    cfFree(app->plat->heap, app, sizeof(*app));
}

//------------------------------------------------------------------------------

static U32
gfxBuildProgram(void)
{
    char const *vtx_shader_src = "#version 330 core\n"
                                 "layout (location = 0) in vec3 aPos;\n"
                                 "void main()\n"
                                 "{\n"
                                 "     gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0);\n"
                                 "}\n";

    char const *pix_shader_src = "#version 330 core\n"
                                 "out vec4 FragColor;\n"
                                 "void main()\n"
                                 "{\n"
                                 "    FragColor = vec4(1.0f, 0.5f, 0.2f, 1.0f);\n"
                                 "}\n";

    I32 success;

    U32 const vtx_shader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vtx_shader, 1, &vtx_shader_src, NULL);
    glCompileShader(vtx_shader);
    glGetShaderiv(vtx_shader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        char info_log[512];
        glGetShaderInfoLog(vtx_shader, CF_ARRAY_SIZE(info_log), NULL, info_log);
        // TODO (Matteo): use IMGUI for logging
        log("Vertex shader compilation error: %s\n", info_log);
    }

    U32 const pix_shader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(pix_shader, 1, &pix_shader_src, NULL);
    glCompileShader(pix_shader);
    glGetShaderiv(pix_shader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        char info_log[512];
        glGetShaderInfoLog(pix_shader, CF_ARRAY_SIZE(info_log), NULL, info_log);
        // TODO (Matteo): use IMGUI for logging
        log("Pixel shader compilation error: %s\n", info_log);
    }

    U32 shader_program = glCreateProgram();
    glAttachShader(shader_program, vtx_shader);
    glAttachShader(shader_program, pix_shader);
    glLinkProgram(shader_program);
    glGetProgramiv(shader_program, GL_LINK_STATUS, &success);
    if (!success)
    {
        char info_log[512];
        glGetProgramInfoLog(pix_shader, CF_ARRAY_SIZE(info_log), NULL, info_log);
        // TODO (Matteo): use IMGUI for logging
        log("Shader program link error: %s\n", info_log);
    }

    glDeleteShader(vtx_shader);
    glDeleteShader(pix_shader);

    return shader_program;
}

static void
gfxInit(GfxState *gfx)
{
    F32 const vertices[] = {-0.5f, -0.5f, 0.0f, //
                            0.5f,  -0.5f, 0.0f, //
                            0.0f,  0.5f,  0.0f};
    I32 const location_in_shader = 0;

    gfx->shader = gfxBuildProgram();

    glGenVertexArrays(1, &gfx->VAO);

    glBindVertexArray(gfx->VAO);

    glGenBuffers(1, &gfx->VBO);
    glBindBuffer(GL_ARRAY_BUFFER, gfx->VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glVertexAttribPointer(location_in_shader, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), 0);
    glEnableVertexAttribArray(location_in_shader);
}

static void
gfxProc(GfxState *gfx)
{
    glUseProgram(gfx->shader);
    glBindVertexArray(gfx->VAO);
    glDrawArrays(GL_TRIANGLES, 0, 3);
}

APP_API APP_UPDATE_PROC(appUpdate)
{
    AppUpdateResult result = {.flags = AppUpdateFlags_None};

    if (igBeginMainMenuBar())
    {
        if (igBeginMenu("File", true)) igEndMenu();

        if (igBeginMenu("Windows", true))
        {
            igMenuItemBoolPtr("Style editor", NULL, &app->windows.style, true);
            igMenuItemBoolPtr("Font options", NULL, &app->windows.fonts, true);
            igSeparator();
            igMenuItemBoolPtr("Stats", NULL, &app->windows.stats, true);
            igMenuItemBoolPtr("Metrics", NULL, &app->windows.metrics, true);
            igEndMenu();
        }

        igEndMainMenuBar();
    }

    if (app->windows.fonts)
    {
        igBegin("Font Options", &app->windows.fonts, 0);

        if (guiFontOptionsEdit(opts))
        {
            result.flags |= AppUpdateFlags_RebuildFonts;
        }

        igEnd();
    }

    if (app->windows.stats)
    {
        Platform *plat = app->plat;
        F64 framerate = (F64)igGetIO()->Framerate;

        igBegin("Application stats stats", &app->windows.stats, 0);
        igText("Average %.3f ms/frame (%.1f FPS)", 1000.0 / framerate, framerate);
        igText("Allocated %.3fkb in %zu blocks", (F64)plat->heap_size / 1024, plat->heap_blocks);
        igSeparator();
        igText("OpenGL version:\t%s", glGetString(GL_VERSION));
        igText("OpenGL renderer:\t%s", glGetString(GL_RENDERER));
        igText("OpenGL shader version:\t%s", glGetString(GL_SHADING_LANGUAGE_VERSION));
        igEnd();
    }

    if (app->windows.style)
    {
        igBegin("Style Editor", &app->windows.style, 0);
        igShowStyleEditor(NULL);
        igEnd();
    }

    if (app->windows.metrics)
    {
        igShowMetricsWindow(&app->windows.metrics);
    }

    gfxProc(&app->gfx);

    return result;
}
