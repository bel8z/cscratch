#include "api.h"

#include "gui/gui.h"

#include "gl/gload.h"

#include "foundation/common.h"

#include "foundation/color.h"
#include "foundation/maths.h"
#include "foundation/path.h"
#include "foundation/strings.h"

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
    cfPlatform *plat;
    cfAllocator *alloc;

    AppWindows windows;
    Rgba clear_color;
};

//------------------------------------------------------------------------------

APP_API APP_CREATE(appCreate)
{
    CF_UNUSED(argv);
    CF_UNUSED(argc);

    // NOTE (Matteo): Memory comes cleared to 0
    AppState *app = cfAlloc(plat->heap, sizeof(*app));

    app->plat = plat;
    app->alloc = plat->heap;

    app->clear_color = (Rgba){.r = 0.45f, .g = 0.55f, .b = 0.60f, .a = 1.00f};

    // Init Dear Imgui
    guiInit(plat->gui);

    // Init OpenGl
    gloadInit(plat->gl);

    return app;
}

APP_API APP_DESTROY(appDestroy)
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

    F32 const amp = size.y / 4;

    F32 const x_offset = 2 * amp;
    F32 const x_space = size.x - x_offset;
    F32 const y_offset = size.y / 2;

    F32 const x_step = x_space / (CF_ARRAY_SIZE(points) - 1);
    F32 const pi2 = 2 * cfAcos(-1.0f);
    F32 const rad_step = pi2 / (CF_ARRAY_SIZE(points) - 1);
    F32 const phase = pi2 * (F32)time;

    ImVec2 const center = {p0.x + amp, p0.y + y_offset};
    ImVec2 polar = {0};

    for (Usize i = 0; i < CF_ARRAY_SIZE(points); ++i)
    {
        F32 rad = i * rad_step + phase;
        F32 sin = amp * cfSin(rad);
        F32 cos = amp * cfCos(rad);

        points[i].x = p0.x + x_offset + x_step * i;
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
}

static void
fxWindow()
{
    ImGuiIO *io = igGetIO();
    igBegin("FX", NULL, ImGuiWindowFlags_AlwaysAutoResize);
    ImVec2 size = {320.0f, 180.0f};
    igInvisibleButton("canvas", size, ImGuiButtonFlags_None);
    ImVec2 p0, p1;
    igGetItemRectMin(&p0);
    igGetItemRectMax(&p1);

    ImVec4 mouse_data;
    mouse_data.x = (io->MousePos.x - p0.x) / size.x;
    mouse_data.y = (io->MousePos.y - p0.y) / size.y;
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

APP_API APP_UPDATE(appUpdate)
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

        if (guiFontOptions(opts))
        {
            result.flags |= AppUpdateFlags_RebuildFonts;
        }

        igEnd();
    }

    if (app->windows.stats)
    {
        cfPlatform *plat = app->plat;
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
    igColorEdit4("clear color", app->clear_color.channel, 0);
    igSeparator();
    guiClock(app->plat->clock());
    igSeparator();
    igText("OpenGL version:\t%s", glGetString(GL_VERSION));
    igText("OpenGL renderer:\t%s", glGetString(GL_RENDERER));
    igText("OpenGL shader version:\t%s", glGetString(GL_SHADING_LANGUAGE_VERSION));
    igEnd();

    fxWindow();

    result.back_color = rgbaPack32(app->clear_color);

    return result;
}
