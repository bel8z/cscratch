#include "api.h"

#include "gui.h"

#include "foundation/allocator.h"
#include "foundation/color.h"
#include "foundation/common.h"
#include "foundation/path.h"
#include "foundation/strings.h"
#include "foundation/util.h"
#include "foundation/vec.h"

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

    AppPaths paths;

    AppWindows windows;
    Rgba clear_color;
};

//------------------------------------------------------------------------------

AppState *
appCreate(cfPlatform *plat, AppPaths paths, char const *argv[], I32 argc)
{
    CF_UNUSED(argv);
    CF_UNUSED(argc);

    // NOTE (Matteo): Memory comes cleared to 0
    AppState *app = cfAlloc(plat->heap, sizeof(*app));

    app->plat = plat;
    app->alloc = plat->heap;

    app->clear_color = (Rgba){.r = 0.45f, .g = 0.55f, .b = 0.60f, .a = 1.00f};

    app->paths = paths;

    return app;
}

void
appDestroy(AppState *app)
{
    cfFree(app->alloc, app, sizeof(*app));
}

//------------------------------------------------------------------------------

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

AppUpdateResult
appUpdate(AppState *state, FontOptions *font_opts)
{
    AppUpdateResult result = {.flags = AppUpdateFlags_None};

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

    if (state->windows.demo) igShowDemoWindow(&state->windows.demo);

    if (state->windows.fonts)
    {
        igBegin("Font Options", &state->windows.fonts, 0);

        if (guiFontOptions(font_opts))
        {
            result.flags |= AppUpdateFlags_RebuildFonts;
        }

        igEnd();
    }

    if (state->windows.stats)
    {
        cfPlatform *plat = state->plat;
        F64 framerate = (F64)igGetIO()->Framerate;

        igBegin("Application stats stats", &state->windows.stats, 0);
        igText("Average %.3f ms/frame (%.1f FPS)", 1000.0 / framerate, framerate);
        igText("Allocated %.3fkb in %zu blocks", (F64)plat->heap_size / 1024, plat->heap_blocks);
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

    static F32 f = 0;

    igCheckbox("Demo Window", &state->windows.demo);
    igSliderFloat("float", &f, 0.0f, 1.0f, "%.3f", 0);
    igColorEdit4("clear color", state->clear_color.channel, 0);
    guiClock(state->plat->clock());
    igEnd();

    result.back_color = rgbaPack32(state->clear_color);

    return result;
}
