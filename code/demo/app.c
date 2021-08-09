#include "api.h"

#include "foundation/colors.h"
#include "foundation/log.h"
#include "foundation/maths.h"
#include "foundation/strings.h"
#include "foundation/time.h"

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

struct AppState
{
    Platform *plat;

    MemAllocator alloc;

    AppWindows windows;
    Rgba32 clear_color;

    CfLog log;
    Duration log_time;
};

//------------------------------------------------------------------------------

APP_API APP_CREATE_PROC(appCreate)
{
    CF_UNUSED(argv);
    CF_UNUSED(argc);

    // NOTE (Matteo): Memory comes cleared to 0
    AppState *app = memAlloc(plat->heap, sizeof(*app));

    app->plat = plat;
    app->alloc = plat->heap;
    app->clear_color = RGBA32_SOLID(115, 140, 153); // R = 0.45, G = 0.55, B = 0.60
    app->windows.stats = true;

    app->log = cfLogCreate(plat->vm, 128);

    while (app->log.write_pos < app->log.buffer.size)
    {
        cfLogAppendC(&app->log, "Filling log buffer\n");
    }

    cfLogAppendC(&app->log, "Filling log buffer\n");

    // Init Dear Imgui
    guiInit(plat->gui);

    // Init OpenGl
    gloadInit(plat->gl);

    return app;
}

APP_API APP_PROC(appDestroy)
{
    cfLogDestroy(&app->log, app->plat->vm);
    cfMemFree(app->alloc, app, sizeof(*app));
}

//------------------------------------------------------------------------------

static void
fxDraw(ImDrawList *draw_list, ImVec2 p0, ImVec2 p1, ImVec2 size, ImVec4 mouse_data, F64 time)
{
    CF_UNUSED(mouse_data);

    ImDrawList_AddRect(draw_list, p0, p1, RGBA32_PURPLE, 0.0f, 0, 1.0f);

    Char8 buffer[1024];
    strPrintf(buffer, CF_ARRAY_SIZE(buffer), "%f", time);
    ImDrawList_AddText_Vec2(draw_list, p0, RGBA32_RED, buffer, buffer + strLength(buffer));

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
guiClock(Duration time)
{
    I64 const secs_per_hour = 60 * 60;
    I64 const secs_per_day = secs_per_hour * 24;
    I64 const secs = time.seconds;
    I64 const ms_remainder = (time.nanos) / 1000000;

    // Euclidean reminder to compute the number of seconds in a day boundary
    I64 total_secs = ((secs % secs_per_day) + secs_per_day) % secs_per_day;
    I64 hours = total_secs / secs_per_hour;
    I64 mins = (total_secs - hours * secs_per_hour) / 60;
    I64 final_secs = total_secs - mins * 60;

    igText("%02d:%02d:%02d.%03d", hours, mins, final_secs, ms_remainder);
}

// static void
// logClear(LogBuffer *log)
// {
//     log->write_pos = 0;
//     memClear(log, CF_ARRAY_SIZE(log->buffer));
// }

// static void
// logWrite(LogBuffer *log, Cstr str, Time time)
// {
//     Usize buf_size = CF_ARRAY_SIZE(log->buffer);
//     Usize str_len = strLength(str);
//     Usize residual = buf_size - log->write_pos;

//     if (residual < str_len + 1)
//     {
//         memClear(log->buffer + log->write_pos, residual);
//         log->write_pos = 0;
//     }

//     if (str_len >= buf_size - 1)
//     {
//         str += str_len - buf_size + 1;
//         str_len = buf_size - 1;
//     }

//     memCopy(str, log->buffer + log->write_pos, str_len);
//     log->write_pos += str_len % buf_size;

//     log->buffer[log->write_pos] = 0;
//     log->time = time;
// }

APP_API APP_UPDATE_PROC(appUpdate)
{
    if (igBeginMainMenuBar())
    {
        if (igBeginMenu("File", true)) igEndMenu();

        if (igBeginMenu("Windows", true))
        {
            igMenuItem_BoolPtr("Style editor", NULL, &state->windows.style, true);
            igMenuItem_BoolPtr("Font options", NULL, &state->windows.fonts, true);
            igSeparator();
            igMenuItem_BoolPtr("Stats", NULL, &state->windows.stats, true);
            igMenuItem_BoolPtr("Metrics", NULL, &state->windows.metrics, true);
            igSeparator();
            igMenuItem_BoolPtr("Demo window", NULL, &state->windows.demo, true);
            igEndMenu();
        }

        igEndMainMenuBar();
    }

    if (state->windows.demo) igShowDemoWindow(&state->windows.demo);

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

        igBegin("Application stats stats", &state->windows.stats,
                ImGuiWindowFlags_HorizontalScrollbar);
        igText("Average %.3f ms/frame (%.1f FPS)", 1000.0 / framerate, framerate);
        igSeparator();
        igText("Allocated %.3fkb in %zu blocks", (F64)plat->heap_size / 1024, plat->heap_blocks);
        igText("Virtual memory reserved %.3fkb - committed %.3fkb", (F64)plat->reserved_size / 1024,
               (F64)plat->committed_size / 1024);
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

    igBegin("Test", NULL, 0);
    {
        static F32 f = 0;

        Duration t = state->plat->clock();

        igCheckbox("Demo Window", &state->windows.demo);
        igSliderFloat("float", &f, 0.0f, 1.0f, "%.3f", 0);
        guiColorEdit("clear color", &state->clear_color);
        igSeparator();
        igCheckbox("Continuous update", &io->continuous_update);
        guiSameLine();
        igCheckbox("Fullscreen", &io->fullscreen);
        igSeparator();
        guiClock(t);
        igSeparator();

        if (timeIsGe(timeSub(t, state->log_time), timeDurationMs(1000)))
        {
            state->log_time = t;
            cfLogAppendF(&state->log, "One second passed\n");
        }

        guiLogBox(&state->log, false);
    }
    igEnd();

    fxWindow();

    io->back_color = state->clear_color;
}
