#include "api.h"

#include "foundation/colors.h"
#include "foundation/maths.h"
#include "foundation/strings.h"

#include "gl/gload.h"

#include "gui/gui.h"

typedef struct AppWindows
{
    bool metrics;
    bool stats;
    bool fonts;
    bool style;
} AppWindows;

enum
{
    CURR_DIR_SIZE = 256,
};

struct AppState
{
    Platform *plat;
    cfAllocator alloc;
    AppWindows windows;
    Rgba32 clear_color;
};

//------------------------------------------------------------------------------

APP_API APP_CREATE_PROC(appCreate)
{
    CF_UNUSED(argv);
    CF_UNUSED(argc);

    // NOTE (Matteo): Memory comes cleared to 0
    AppState *app = cfAlloc(plat->heap, sizeof(*app));

    app->plat = plat;
    app->alloc = plat->heap;
    app->clear_color = RGBA32_SOLID(115, 140, 153); // R = 0.45, G = 0.55, B = 0.60

    // Init Dear Imgui
    guiInit(plat->gui);

    // Init OpenGl
    gloadInit(plat->gl);

    return app;
}

APP_API APP_PROC(appDestroy)
{
    cfFree(app->alloc, app, sizeof(*app));
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
    U32 vtx_shader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vtx_shader, 1, &vtx_shader_src, NULL);
    glCompileShader(vtx_shader);

    I32 success;
    glGetShaderiv(vtx_shader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        char info_log[512];
        glGetShaderInfoLog(vtx_shader, CF_ARRAY_SIZE(info_log), NULL, info_log);
        // TODO (Matteo): use IMGUI for logging
        printf("Vertex shader compilation error: %s\n", info_log);
    }

    U32 pix_shader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(pix_shader, 1, &pix_shader_src, NULL);
    glCompileShader(pix_shader);

    glGetShaderiv(pix_shader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        char info_log[512];
        glGetShaderInfoLog(pix_shader, CF_ARRAY_SIZE(info_log), NULL, info_log);
        // TODO (Matteo): use IMGUI for logging
        printf("Pixel shader compilation error: %s\n", info_log);
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
        printf("Shader program link error: %s\n", info_log);
    }

    glDeleteShader(vtx_shader);
    glDeleteShader(pix_shader);

    return shader_program;
}

static void
gfxProc(void)
{
    F32 const vertices[] = {-0.5f, -0.5f, 0.0f, //
                            0.5f,  -0.5f, 0.0f, //
                            0.0f,  0.5f,  0.0f};
    I32 const location_in_shader = 0;

    static U32 VBO = 0;
    static U32 shader_program = 0;
    static U32 VAO = 0;

    if (!VBO) glGenBuffers(1, &VBO);
    if (!shader_program) shader_program = gfxBuildProgram();

    if (!VAO)
    {
        // ..:: Initialization code (done once (unless your object frequently changes)) :: ..
        glGenVertexArrays(1, &VAO);

        // 1. bind Vertex Array Object
        glBindVertexArray(VAO);
        // 2. copy our vertices array in a buffer for OpenGL to use
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
        // 3. then set our vertex attributes pointers
        glVertexAttribPointer(location_in_shader, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), 0);
        glEnableVertexAttribArray(location_in_shader);
    }

    // ..:: Drawing code (in render loop) :: ..
    // 4. draw the object
    glUseProgram(shader_program);
    glBindVertexArray(VAO);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    // someOpenGLFunctionThatDrawsOurTriangle();
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

    gfxProc();

    return result;
}
