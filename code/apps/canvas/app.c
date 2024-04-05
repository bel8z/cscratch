#include "app.h"
#include "platform.h"

#include "foundation/colors.h"
#include "foundation/log.h"
#include "foundation/math.inl"
#include "foundation/memory.h"
#include "foundation/strings.h"
#include "foundation/time.h"

#include "gl/gl_api.h"

#include "gui/gui.h"

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

    MemAllocator alloc;

    AppWindows windows;
    Srgb32 clear_color;
    bool srgb_framebuffer;

    CfLog log;
    Duration log_time;
};

static CfLog *g_log = NULL;
static Clock *g_clock = NULL;

#define appLog(...) (g_log ? (cfLogAppendF(g_log, __VA_ARGS__), 1) : 0)

//------------------------------------------------------------------------------

APP_API
APP_CREATE_FN(appCreate)
{
    CF_UNUSED(cmd_line);

    // NOTE (Matteo): Memory comes cleared to 0
    AppState *app = memAllocStruct(plat->heap, AppState);

    app->plat = plat;
    app->alloc = plat->heap;
    app->clear_color = SRGB32_SOLID(115, 140, 153); // R = 0.45, G = 0.55, B = 0.60
    app->windows.stats = true;
    app->log = cfLogCreate(plat->vmem, 128);

    appLoad(app);

    return app;
}

APP_API
APP_FN(appDestroy)
{
    appUnload(app);
    cfLogDestroy(&app->log, app->plat->vmem);
    memFreeStruct(app->alloc, app);
}

APP_API
APP_FN(appLoad)
{
    CF_ASSERT_NOT_NULL(app);
    CF_ASSERT_NOT_NULL(app->plat);

    // Init globals
    g_log = &app->log;
    g_clock = &app->plat->clock;

    // Init Dear Imgui
    guiSetContext(app->plat->gui);

    // Init OpenGl
    glApiSet(app->plat->gl);

    glDisable(GL_FRAMEBUFFER_SRGB);
}

APP_API
APP_FN(appUnload)
{
    CF_ASSERT_NOT_NULL(app);
    g_log = NULL;
}

//------------------------------------------------------------------------------

static inline Vec2f
pointToScreen(GuiCanvas *canvas, Vec2f point)
{
    return (Vec2f){
        .x = canvas->size.x * 0.5f + point.x + canvas->p0.x,
        .y = canvas->size.y * 0.5f - point.y + canvas->p0.y,
    };
}

static inline Vec2f
pointToCanvas(GuiCanvas *canvas, Vec2f point)
{
    return (Vec2f){
        .x = point.x - canvas->size.x * 0.5f - canvas->p0.x,
        .y = canvas->size.y * 0.5f - point.y - canvas->p0.y,
    };
}

static void
canvasPutPixel(GuiCanvas *canvas, Vec2f pixel, Srgb32 color)
{
    Vec2f p0 = pointToScreen(canvas, pixel);
    Vec2f p1 = {.x = p0.x + 1, .y = p0.y + 1};
    canvas->fill_color = color;
    guiCanvasFillRect(canvas, p0, p1);
}

static void
canvasWindow(void)
{
    (void)pointToCanvas;

    Duration time = clockElapsed(g_clock);
    Char8 buffer[1024];
    strPrint(buffer, CF_ARRAY_SIZE(buffer), "%f", timeGetSeconds(time));

    Vec2f mouse = guiGetMousePos();
    float mouse_l = guiGetMouseDownDuration(GuiMouseButton_Left);
    float mouse_r = guiGetMouseDownDuration(GuiMouseButton_Right);

    CF_UNUSED(mouse);
    CF_UNUSED(mouse_l);
    CF_UNUSED(mouse_r);

    GuiCanvas canvas = {0};
    guiSetNextWindowSize((Vec2f){{640, 360}}, GuiCond_Once);
    guiBegin("Canvas", NULL);
    guiCanvasBegin(&canvas);

    canvas.stroke_color = SRGB32_BLACK;
    guiCanvasDrawRect(&canvas, canvas.p0, canvas.p1);
    guiCanvasDrawText(&canvas, strFromCstr(buffer), canvas.p0, canvas.stroke_color);

    canvasPutPixel(&canvas, (Vec2f){{0, 0}}, SRGB32_RED);

    guiCanvasEnd(&canvas);
    guiEnd();
}

static void
guiClock(Duration time)
{
    I64 const secs_per_hour = 60 * 60;
    I64 const secs_per_day = secs_per_hour * 24;

    // Euclidean reminder to compute the number of seconds in a day boundary
    I64 total_secs = mModEuclid(time.seconds, secs_per_day);
    I64 hours = total_secs / secs_per_hour;
    I64 mins = (total_secs - hours * secs_per_hour) / 60;
    I64 final_secs = total_secs - mins * 60;

    guiText("%02lld:%02lld:%02lld.%09u", hours, mins, final_secs, time.nanos);
}

APP_API
APP_UPDATE_FN(appUpdate)
{
    Platform *plat = state->plat;

    if (guiBeginMainMenuBar())
    {
        if (guiBeginMenu("File", true)) guiEndMenu();

        if (guiBeginMenu("Windows", true))
        {
            guiMenuItem("Style editor", &state->windows.style);
            guiMenuItem("Font options", &state->windows.fonts);
            guiSeparator();
            guiMenuItem("Stats", &state->windows.stats);
            guiMenuItem("Metrics", &state->windows.metrics);
            guiEndMenu();
        }

        guiEndMainMenuBar();
    }

    guiDockSpace(GuiDockStyle_Transparent);

    if (state->windows.fonts)
    {
        guiBegin("Font Options", &state->windows.fonts);
        io->rebuild_fonts = guiFontOptionsEdit(io->font_opts);
        guiEnd();
    }

    if (state->windows.stats)
    {
        double framerate = (double)guiGetFramerate();
        GuiMemory gui_mem = guiMemoryInfo();

        guiBegin("Application stats", &state->windows.stats);
        guiText("Average %.3f ms/frame (%.1f FPS)", 1000.0 / framerate, framerate);

        guiSeparator();
        guiText("Virtual memory: Reserved %.3fkb - Committed %.3fkb",
                (double)plat->reserved_size / 1024, (double)plat->committed_size / 1024);
        guiText("Heap memory   : Allocated %.3fkb in %zu blocks", (double)plat->heap_size / 1024,
                plat->heap_blocks);
        guiText("GUI memory    : Allocated %.3fkb in %zu blocks", (double)gui_mem.size / 1024,
                gui_mem.blocks);

        guiSeparator();
        guiText("OpenGL version:\t%s", glGetString(GL_VERSION));
        guiText("OpenGL renderer:\t%s", glGetString(GL_RENDERER));
        guiText("OpenGL shader version:\t%s", glGetString(GL_SHADING_LANGUAGE_VERSION));
        guiEnd();
    }

    if (state->windows.style)
    {
        guiBegin("Style Editor", &state->windows.style);
        guiStyleEditor();
        guiEnd();
    }

    if (state->windows.metrics)
    {
        guiMetricsWindow(&state->windows.metrics);
    }

    guiBegin("Options", NULL);
    {
        Clock *clock = &plat->clock;
        Duration t = clockElapsed(clock);

        guiColorEdit("clear color", &state->clear_color);
        guiSeparator();
        guiThemeSelector("Theme");
        guiCheckbox("Continuous update", &io->continuous_update);
        guiSameLine();
        guiCheckbox("Fullscreen", &io->fullscreen);
        guiSameLine();
        if (guiCheckbox("Srgb", &state->srgb_framebuffer))
        {
            guiGammaCorrection(state->srgb_framebuffer);
        }
        guiSeparator();
        guiClock(t);
        guiSeparator();

        guiLogBox(&state->log, false);
    }
    guiEnd();

    if (state->srgb_framebuffer)
    {
        glEnable(GL_FRAMEBUFFER_SRGB);
    }
    else
    {
        glDisable(GL_FRAMEBUFFER_SRGB);
    }

    canvasWindow();

    io->back_color = state->clear_color;
}
