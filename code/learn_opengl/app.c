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

    return result;
}
