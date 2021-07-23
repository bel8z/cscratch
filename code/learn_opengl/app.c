#include "api.h"

#include "foundation/colors.h"
#include "foundation/maths.h"
#include "foundation/strings.h"

#include "gl/gload.h"

#include "gui/gui.h"

typedef struct GfxState
{
    U32 VBO, EBO, VAO, shader;
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

// F32 const vertices[] = {-0.5f, -0.5f, 0.0f, //
//                         0.5f,  -0.5f, 0.0f, //
//                         0.0f,  0.5f,  0.0f};

F32 const vertices[] = {
    0.5f,  0.5f,  0.0f, // top right
    0.5f,  -0.5f, 0.0f, // bottom right
    -0.5f, -0.5f, 0.0f, // bottom left
    -0.5f, 0.5f,  0.0f  // top left
};

U32 const indices[] = {
    // note that we start from 0!
    0, 1, 3, // first triangle
    1, 2, 3  // second triangle
};

//------------------------------------------------------------------------------

#define log(...) fprintf(stderr, __VA_ARGS__)

static void gfxInit(GfxState *state);

//------------------------------------------------------------------------------

APP_API APP_CREATE_PROC(appCreate)
{
    CF_UNUSED(argv);
    CF_UNUSED(argc);

    // NOTE (Matteo): Memory comes cleared to 0
    AppState *app = cfMemAlloc(plat->heap, sizeof(*app));

    app->plat = plat;
    app->clear_color = RGBA32_SOLID(115, 140, 153); // R = 0.45, G = 0.55, B = 0.60

    appLoad(app);

    gfxInit(&app->gfx);

    return app;
}

APP_API APP_PROC(appLoad)
{
    CF_ASSERT_NOT_NULL(app);
    CF_ASSERT_NOT_NULL(app->plat);

    // Init Dear Imgui
    guiInit(app->plat->gui);

    // Init OpenGl
    gloadInit(app->plat->gl);
    if (!gloadIsSupported(3, 3))
    {
        // TODO (Matteo): Log using IMGUI
        log("OpenGL 3.3 is not supported!");
    }
}

APP_API APP_PROC(appUnload)
{
    CF_ASSERT_NOT_NULL(app);
}

APP_API APP_PROC(appDestroy)
{
    CF_ASSERT_NOT_NULL(app);
    CF_ASSERT_NOT_NULL(app->plat);

    cfMemFree(app->plat->heap, app, sizeof(*app));
}

//------------------------------------------------------------------------------

static U32
gfxBuildProgram(void)
{
    Cstr vtx_shader_src =
        "#version 330 core\n"
        "layout (location = 0) in vec3 aPos;\n" // the position variable has attribute position 0
        "out vec4 vertexColor;\n"               // specify a color output to the fragment shader
        "void main()\n"
        "{\n"
        "     gl_Position = vec4(aPos.xyz, 1.0);\n"
        "     vertexColor = vec4(0.5, 0.0, 0.0, 1.0);\n" // set the output variable to a dark-red
                                                         // color
        "}\n";

    Cstr pix_shader_src = "#version 330 core\n"
                          "out vec4 FragColor;\n"
                          "in vec4 vertexColor;\n"   // the input variable from the vertex shader
                                                     // (same name and same type)
                          "uniform vec4 ourColor;\n" // we set this variable in the OpenGL code.
                          "void main()\n"
                          "{\n"
                          "    FragColor = ourColor;\n"
                          "}\n";

    I32 success;

    U32 const vtx_shader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vtx_shader, 1, &vtx_shader_src, NULL);
    glCompileShader(vtx_shader);
    glGetShaderiv(vtx_shader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        Char8 info_log[512];
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
        Char8 info_log[512];
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
        Char8 info_log[512];
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

    I32 const location_in_shader = 0;

    gfx->shader = gfxBuildProgram();

    glGenVertexArrays(1, &gfx->VAO);
    glGenBuffers(1, &gfx->VBO);
    glGenBuffers(1, &gfx->EBO);

    // Bind the Vertex Array Object first, then bind and set vertex buffer(s), and then configure
    // vertex attributes(s).
    glBindVertexArray(gfx->VAO);

    glBindBuffer(GL_ARRAY_BUFFER, gfx->VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glVertexAttribPointer(location_in_shader, CF_ARRAY_SIZE(vertices) / 3, GL_FLOAT, GL_FALSE,
                          3 * sizeof(float), 0);
    glEnableVertexAttribArray(location_in_shader);

    // note that this is allowed, the call to glVertexAttribPointer registered VBO as the vertex
    // attribute's bound vertex buffer object so afterwards we can safely unbind
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gfx->EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    // You can unbind the VAO afterwards so other VAO calls won't accidentally modify this VAO, but
    // this rarely happens.
    // Modifying other VAOs requires a call to glBindVertexArray anyways so we generally don't
    // unbind VAOs (nor VBOs) when it's not directly necessary.
    glBindVertexArray(0);

    // The EBO must be unbound AFTER unbinding the VAO in order to keep it recorded
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

static void
gfxProc(GfxState *gfx)
{
    glUseProgram(gfx->shader);
    glBindVertexArray(gfx->VAO);
    // glDrawArrays(GL_TRIANGLES, 0, CF_ARRAY_SIZE(vertices) / 3);
    // glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    F32 time_value = (F32)igGetTime();
    Rgba color = {.g = (cfSin(time_value) / 2.0f) + 0.5f, .a = 1.0f};
    I32 color_location = glGetUniformLocation(gfx->shader, "ourColor");
    if (color_location >= 0)
    {
        glUniform4fv(color_location, 1, color.channel);
    }

    glDrawElements(GL_TRIANGLES, CF_ARRAY_SIZE(indices), GL_UNSIGNED_INT, 0);
}

APP_API APP_UPDATE_PROC(appUpdate)
{
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
            igEndMenu();
        }

        igEndMainMenuBar();
    }

    if (state->windows.fonts)
    {
        igBegin("Font Options", &state->windows.fonts, 0);
        io->rebuild_fonts = guiFontOptionsEdit(io->font_opts);
        igEnd();
    }

    if (state->windows.stats)
    {
        Platform *plat = state->plat;
        F64 framerate = (F64)igGetIO()->Framerate;

        igBegin("Application stats stats", &state->windows.stats, 0);
        igText("Average %.3f ms/frame (%.1f FPS)", 1000.0 / framerate, framerate);
        igText("Allocated %.3fkb in %zu blocks", (F64)plat->heap_size / 1024, plat->heap_blocks);
        igSeparator();
        igText("OpenGL version:\t%s", glGetString(GL_VERSION));
        igText("OpenGL renderer:\t%s", glGetString(GL_RENDERER));
        igText("OpenGL shader version:\t%s", glGetString(GL_SHADING_LANGUAGE_VERSION));
        igEnd();
    }

    if (state->windows.style)
    {
        igBegin("Style Editor", &state->windows.style, 0);
        igShowStyleEditor(NULL);
        igEnd();
    }

    if (state->windows.metrics)
    {
        igShowMetricsWindow(&state->windows.metrics);
    }

    gfxProc(&state->gfx);
}
