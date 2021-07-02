#include "api.h"
#include "foundation/color.h"
#include "foundation/common.h"
#include "foundation/maths.h"
#include "foundation/path.h"
#include "foundation/strings.h"
#include "gl/gload.h"
#include "gui/gui.h"

typedef struct AppWindows
{
    bool demo;
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
    cfAllocator *alloc;

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

static void
fxDraw(ImDrawList *draw_list, ImVec2 p0, ImVec2 p1, ImVec2 size, ImVec4 mouse_data, F64 time)
{
    CF_UNUSED(mouse_data);

    ImDrawList_AddRect(draw_list, p0, p1, RGBA32_PURPLE, 0.0f, 0, 1.0f);

    char buffer[1024];
    strPrintf(buffer, 1024, "%f", time);
    ImDrawList_AddTextVec2(draw_list, p0, RGBA32_RED, buffer, buffer + strLength(buffer));

    // 1 Hz sinusoid, YAY!

    ImVec2 points[1024] = {0};

    F32 const amp = cfMin(size.x, size.y) / 4;

    F32 const x_offset = 2 * amp;
    F32 const x_space = size.x - x_offset;
    F32 const x_step = x_space / (CF_ARRAY_SIZE(points) - 1);
    F32 const y_offset = size.y / 2;

    F32 const pi2 = 2 * cfAcos(-1.0f);
    F32 const rad_step = 2 * pi2 / (CF_ARRAY_SIZE(points) - 1);
    F32 const phase = pi2 * (F32)time;

    ImVec2 const center = {p0.x + amp, p0.y + y_offset};
    ImVec2 polar = {0};

    for (Usize i = 0; i < CF_ARRAY_SIZE(points); ++i)
    {
        F32 rad = (F32)i * rad_step + phase;
        F32 sin = amp * cfSin(rad);
        F32 cos = amp * cfCos(rad);

        points[i].x = p0.x + x_offset + x_step * (F32)i;
        points[i].y = p0.y + y_offset + sin;

        polar.x = center.x + cos;
        polar.y = center.y + sin;
    }

    ImDrawList_AddCircle(draw_list, center, amp, RGBA32_YELLOW, 0, 1.0f);
    ImDrawList_AddLine(draw_list, center, polar, RGBA32_YELLOW, 1.0f);
    ImDrawList_AddLine(draw_list, polar, points[0], RGBA32_YELLOW, 1.0f);
    ImDrawList_AddPolyline(draw_list, points, CF_ARRAY_SIZE(points), RGBA32_YELLOW, 0, 1.0f);

    ImDrawList_AddLine(draw_list, (ImVec2){p0.x, p0.y + y_offset}, (ImVec2){p1.x, p0.y + y_offset},
                       RGBA32_PURPLE, 1.0f);
    ImDrawList_AddLine(draw_list, (ImVec2){p0.x + amp, p0.y}, (ImVec2){p0.x + amp, p1.y},
                       RGBA32_PURPLE, 1.0f);
    ImDrawList_AddLine(draw_list, (ImVec2){p0.x + 2 * amp, p0.y}, (ImVec2){p0.x + 2 * amp, p1.y},
                       RGBA32_PURPLE, 1.0f);

    ImDrawList_AddCircleFilled(draw_list, (ImVec2){mouse_data.x, mouse_data.y}, 5.0f,
                               RGBA32_ORANGE_RED, 0);
}

static void
fxWindow()
{
    ImGuiIO *io = igGetIO();
    ImVec2 size, p0, p1;
    igSetNextWindowSize((ImVec2){320, 180}, ImGuiCond_Once);
    igBegin("FX", NULL, 0);
    igGetContentRegionAvail(&size);
    igInvisibleButton("canvas", size, ImGuiButtonFlags_None);
    igGetItemRectMin(&p0);
    igGetItemRectMax(&p1);

    ImVec4 mouse_data;
    mouse_data.x = io->MousePos.x; // (io->MousePos.x - p0.x) / size.x;
    mouse_data.y = io->MousePos.y; // (io->MousePos.y - p0.y) / size.y;
    mouse_data.z = io->MouseDownDuration[0];
    mouse_data.w = io->MouseDownDuration[1];

    ImDrawList *draw_list = igGetWindowDrawList();
    ImDrawList_PushClipRect(draw_list, p0, p1, true);
    fxDraw(draw_list, p0, p1, size, mouse_data, igGetTime());
    ImDrawList_PopClipRect(draw_list);

    igEnd();
}

static void
guiClock(Time time)
{
    I64 const secs_per_hour = 60 * 60;
    I64 const secs_per_day = secs_per_hour * 24;
    I64 const secs = time.nanoseconds / 1000000000;
    I64 const ms_remainder = (time.nanoseconds - secs * 1000000000) / 1000000;

    // Euclidean reminder to compute the number of seconds in a day boundary
    I64 total_secs = ((secs % secs_per_day) + secs_per_day) % secs_per_day;
    I64 hours = total_secs / secs_per_hour;
    I64 mins = (total_secs - hours * secs_per_hour) / 60;
    I64 final_secs = total_secs - mins * 60;

    igText("%02d:%02d:%02d.%03d", hours, mins, final_secs, ms_remainder);
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
            igSeparator();
            igMenuItemBoolPtr("Demo window", NULL, &app->windows.demo, true);
            igEndMenu();
        }

        igEndMainMenuBar();
    }

    if (app->windows.demo) igShowDemoWindow(&app->windows.demo);

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

    igBegin("Test", NULL, 0);

    static F32 f = 0;

    igCheckbox("Demo Window", &app->windows.demo);
    igSliderFloat("float", &f, 0.0f, 1.0f, "%.3f", 0);
    guiColorEdit("clear color", &app->clear_color);
    igSeparator();
    guiClock(app->plat->clock());
    igSeparator();
    igText("OpenGL version:\t%s", glGetString(GL_VERSION));
    igText("OpenGL renderer:\t%s", glGetString(GL_RENDERER));
    igText("OpenGL shader version:\t%s", glGetString(GL_SHADING_LANGUAGE_VERSION));
    igEnd();

    fxWindow();

    result.back_color = app->clear_color;

    return result;
}
