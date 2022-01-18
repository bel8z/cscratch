#include "app.h"
#include "platform.h"

#define SHADER_IMPL
#include "shader.h"

#define IMAGE_IMPL
#include "image.h"

#include "gl/gload.h"

#include "gui/gui.h"

#include "foundation/colors.h"
#include "foundation/error.h"
#include "foundation/log.h"
#include "foundation/math.inl"
#include "foundation/paths.h"
#include "foundation/strings.h"

//------------------------------------------------------------------------------

typedef struct Vertex
{
    F32 x, y, z;
    F32 r, g, b;
    F32 u, v;
} Vertex;

typedef struct Texture
{
    I32 width;
    I32 height;
    U32 id;
} Texture;

typedef struct GfxState
{
    Shader shader;
    U32 VBO, EBO, VAO;
    Texture container, wall;
} GfxState;

struct AppState
{
    Platform *plat;

    GfxState gfx;

    CfLog log;

    // Output
    Srgb32 clear_color;

    // Windows
    bool metrics;
    bool stats;
    bool fonts;
    bool style;
    bool log_box;
};

//------------------------------------------------------------------------------

// NOTE (Matteo): CW vertices

Vertex vertices[] = {
    {0.5f, 0.5f, 0.0f, 0.0f, 0.0f, 1.0f, 1, 1},   // top right
    {0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 0.0f, 1, 0},  // bottom right
    {-0.5f, -0.5f, 0.0f, 0.0f, 1.0f, 0.0f, 0, 0}, // bottom left
    {-0.5f, 0.5f, 0.0f, 1.0f, 1.0f, 0.0f, 0, 1},  // top left
};

U32 indices[] = {
    0, 1, 3, //
    1, 2, 3,
};

Cstr vtx_shader_src = //
    "#version 330 core\n"
    "\n"
    "layout (location = 0) in vec3 vtxPos;\n"
    "layout (location = 1) in vec3 vtxColorIn;\n"
    "layout (location = 2) in vec2 vtxTexCoord;\n"
    "\n"
    "out vec3 vtxColorOut;\n"
    "\n"
    "void main()\n"
    "{\n"
    "     gl_Position = vec4(vtxPos, 1.0);\n"
    "     vtxColorOut = vtxColorIn;\n"
    "}\n";

Cstr pix_shader_src = //
    "#version 330 core\n"
    "\n"
    "in vec3 vtxColorOut;\n"
    "\n"
    "out vec4 FragColor;\n"
    "\n"
    "void main()\n"
    "{\n"
    "    FragColor = vec4(vtxColorOut, 1.0);\n"
    "}\n";

//------------------------------------------------------------------------------

static void gfxInit(GfxState *state, CfLog *log, Paths *paths, IoFileApi *file);
static void gfxShutdown(GfxState *gfx);

//------------------------------------------------------------------------------

APP_API APP_CREATE_PROC(appCreate)
{
    CF_UNUSED(cmd_line);

    // NOTE (Matteo): Memory comes cleared to 0
    AppState *app = memAlloc(plat->heap, sizeof(*app));

    app->plat = plat;
    app->clear_color = SRGB32_SOLID(115, 140, 153); // R = 0.45, G = 0.55, B = 0.60
    app->log = cfLogCreate(plat->vm, CF_MB(1));

    appLoad(app);

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
        cfLogAppendF(&app->log, "OpenGL 3.3 is not supported!");
    }

    // Init image library
    imageInit(app->plat->heap);

    // Init graphics state
    gfxInit(&app->gfx, &app->log, app->plat->paths, app->plat->file);
}

APP_API APP_PROC(appUnload)
{
    CF_ASSERT_NOT_NULL(app);
    gfxShutdown(&app->gfx);
}

APP_API APP_PROC(appDestroy)
{
    CF_ASSERT_NOT_NULL(app);
    CF_ASSERT_NOT_NULL(app->plat);

    cfLogDestroy(&app->log, app->plat->vm);
    memFree(app->plat->heap, app, sizeof(*app));
}

//------------------------------------------------------------------------------

static bool
textureLoad(Texture *tex, Str filename, IoFileApi *file)
{
    Image image = {0};
    if (!imageLoadFromFile(&image, filename, file)) return false;

    tex->width = image.width;
    tex->height = image.height;

    glGenTextures(1, &tex->id);
    glBindTexture(GL_TEXTURE_2D, tex->id);

    // set the texture wrapping/filtering options (on the currently bound texture object)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Generate the texture (default mimpmap)
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, tex->width, tex->height, 0, GL_RGBA, GL_UNSIGNED_BYTE,
                 image.bytes);

    // Let OpenGL generate the mipmaps for the current texture
    glGenerateMipmap(GL_TEXTURE_2D);

    imageUnload(&image);

    return true;
}

static void
gfxInit(GfxState *gfx, CfLog *log, Paths *paths, IoFileApi *file)
{
    //=== Shaders ==//

    gfx->shader = shaderLoadStrings(strFromCstr(vtx_shader_src), //
                                    strFromCstr(pix_shader_src), //
                                    log);

    //=== Buffers ==//

    glGenVertexArrays(1, &gfx->VAO);
    glGenBuffers(1, &gfx->VBO);
    glGenBuffers(1, &gfx->EBO);

    // Bind the Vertex Array Object first, then bind and set vertex buffer(s), and then configure
    // vertex attributes(s).
    glBindVertexArray(gfx->VAO);

    glBindBuffer(GL_ARRAY_BUFFER, gfx->VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    // Position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), 0);
    glEnableVertexAttribArray(0);

    // Color attribure
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)offsetof(Vertex, r));
    glEnableVertexAttribArray(1);

    // Texcoord attribute
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)offsetof(Vertex, u));
    glEnableVertexAttribArray(2);

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

    //=== Textures ==//

    StrBuffer filename = {0};
    strBufferInit(&filename);
    strBufferAppendStr(&filename, paths->data);
    Usize pos = filename.str.len;

    filename.str.len = pos;
    strBufferAppendStr(&filename, strLiteral("container.jpg"));
    textureLoad(&gfx->container, filename.str, file);

    filename.str.len = pos;
    strBufferAppendStr(&filename, strLiteral("wall.jpg"));
    textureLoad(&gfx->wall, filename.str, file);
}

static void
gfxShutdown(GfxState *gfx)
{
    glBindTexture(GL_TEXTURE_2D, 0);
    glDeleteTextures(1, &gfx->container.id);
    glDeleteTextures(1, &gfx->wall.id);

    glBindVertexArray(0);
    glDeleteBuffers(1, &gfx->EBO);
    glDeleteBuffers(1, &gfx->VBO);
    glDeleteVertexArrays(1, &gfx->VAO);

    shaderClear();
    shaderUnload(&gfx->shader);
}

static void
gfxProc(GfxState *gfx, CfLog *log)
{
    CF_UNUSED(log); // at the moment

    shaderBind(gfx->shader);
    glBindVertexArray(gfx->VAO);
    glDrawElements(GL_TRIANGLES, CF_ARRAY_SIZE(indices), GL_UNSIGNED_INT, 0);
}

APP_API APP_UPDATE_PROC(appUpdate)
{
    if (guiBeginMainMenuBar())
    {
        if (guiBeginMenu("File", true)) guiEndMenu();

        if (guiBeginMenu("Windows", true))
        {
            guiMenuItem("Style editor", &state->style);
            guiMenuItem("Font options", &state->fonts);
            guiSeparator();
            guiMenuItem("Log", &state->log_box);
            guiMenuItem("Stats", &state->stats);
            guiMenuItem("Metrics", &state->metrics);
            guiEndMenu();
        }

        guiEndMainMenuBar();
    }

    if (state->fonts)
    {
        guiBegin("Font Options", &state->fonts);
        io->rebuild_fonts = guiFontOptionsEdit(io->font_opts);
        guiEnd();
    }

    if (state->stats)
    {
        Platform *plat = state->plat;
        F64 framerate = (F64)guiGetFramerate();

        guiBegin("Application stats stats", &state->stats);
        guiText("Average %.3f ms/frame (%.1f FPS)", 1000.0 / framerate, framerate);
        guiText("Allocated %.3fkb in %zu blocks", (F64)plat->heap_size / 1024, plat->heap_blocks);
        guiSeparator();
        guiText("OpenGL version:\t%s", glGetString(GL_VERSION));
        guiText("OpenGL renderer:\t%s", glGetString(GL_RENDERER));
        guiText("OpenGL shader version:\t%s", glGetString(GL_SHADING_LANGUAGE_VERSION));
        guiEnd();
    }

    if (state->style)
    {
        guiBegin("Style Editor", &state->style);
        guiStyleEditor();
        guiEnd();
    }

    if (state->log_box)
    {
        guiBegin("Log", &state->log_box);
        guiSetNextWindowSize((Vec2){.x = 500, .y = 400}, GuiCond_FirstUseEver);
        guiLogBox(&state->log, true);
        guiEnd();
    }

    if (state->metrics)
    {
        guiMetricsWindow(&state->metrics);
    }

    gfxProc(&state->gfx, &state->log);
}
