#include "api.h"

#include "log.h"
#define SHADER_IMPL
#include "shader.h"

#include "gl/gload.h"

#include "gui/gui.h"

#include "foundation/colors.h"
#include "foundation/fs.h"
#include "foundation/maths.h"
#include "foundation/strings.h"

//------------------------------------------------------------------------------

typedef struct GfxState
{
    Shader shader;
    U32 VBO, EBO, VAO;
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

float vertices[] = {
    // positions        // colors
    0.5f,  -0.5f, 0.0f, 1.0f, 0.0f, 0.0f, // bottom right
    -0.5f, -0.5f, 0.0f, 0.0f, 1.0f, 0.0f, // bottom left
    0.0f,  0.5f,  0.0f, 0.0f, 0.0f, 1.0f  // top
};

Cstr vtx_shader_src =
    "#version 330 core\n"
    "layout (location = 0) in vec3 aPos;\n"   // the position variable has attribute position 0
    "layout (location = 1) in vec3 aColor;\n" // the color  variable has attribute position 1
    "out vec3 ourColor;\n"                    // output a color to the fragment shader
    "void main()\n"
    "{\n"
    "     gl_Position = vec4(aPos.xyz, 1.0);\n"
    "     ourColor = aColor;\n" // set ourColor to the input color we got from the vertex data
    "}\n";

Cstr pix_shader_src = "#version 330 core\n"
                      "out vec4 FragColor;\n"
                      "in vec3 ourColor;\n" // we set this variable in the OpenGL code.
                      "void main()\n"
                      "{\n"
                      "    FragColor = vec4(ourColor, 1.0);\n"
                      "}\n";

//------------------------------------------------------------------------------

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
        appLog("OpenGL 3.3 is not supported!");
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

static void
gfxInit(GfxState *gfx)
{
    gfx->shader = shaderLoadStrings(strFromCstr(vtx_shader_src), //
                                    strFromCstr(pix_shader_src));

    glGenVertexArrays(1, &gfx->VAO);
    glGenBuffers(1, &gfx->VBO);
    glGenBuffers(1, &gfx->EBO);

    // Bind the Vertex Array Object first, then bind and set vertex buffer(s), and then configure
    // vertex attributes(s).
    glBindVertexArray(gfx->VAO);

    glBindBuffer(GL_ARRAY_BUFFER, gfx->VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    I32 stride = 6 * sizeof(float);
    Iptr color_offset = stride / 2;

    // Position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, 0);
    glEnableVertexAttribArray(0);

    // Color attribure
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void *)color_offset);
    glEnableVertexAttribArray(1);

    // note that this is allowed, the call to glVertexAttribPointer registered VBO as the vertex
    // attribute's bound vertex buffer object so afterwards we can safely unbind
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    // glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gfx->EBO);
    // glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

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
    shaderBegin(gfx->shader);
    glBindVertexArray(gfx->VAO);
    glDrawArrays(GL_TRIANGLES, 0, 3);
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
