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

typedef struct Pwm
{
    F64 frequency, duty_cyle;
} Pwm;

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

    Pwm pwm;
    F64 phase;
};

CF_INTERNAL F64
pwmSample(Pwm const *pwm, F64 t)
{
    // NOTE (Matteo): Derived from the subtraction of two phase shifted sawtooth
    // waves, defined as y(t) = f*t - floor(f*t), where the phase shift is
    // determined by the duty cycle.
    F64 ft = pwm->frequency * t;
    return mFloor(ft) - mFloor(ft - pwm->duty_cyle);
}

CF_INTERNAL void
pwmWindow(AppState *app)
{
    static DVec3 samples[512] = {0};

    Pwm *pwm = &app->pwm;

    F64 period = 1 / pwm->frequency;

    guiBegin("PWM", NULL);
    guiSliderF64("Duty cycle", &pwm->duty_cyle, 0, 1);
    guiSliderF64("Phase shift", &app->phase, 0, period * 0.5);

    F64 t_max = 3 * period;
    F64 t_step = t_max / (CF_ARRAY_SIZE(samples) - 1);

    for (Usize i = 0; i < CF_ARRAY_SIZE(samples); ++i)
    {
        F64 t = i * t_step;
        samples[i].x = t;
        samples[i].y = pwmSample(pwm, t);
        samples[i].z = pwmSample(pwm, t - app->phase);
    }

    GuiPlotSetup plot = {
        .info[GuiAxis_X1] = &(GuiAxisInfo){.range =
                                               &(GuiAxisRange){
                                                   .max = t_max,
                                                   .locked = true,
                                               }},
        .info[GuiAxis_Y1] = &(GuiAxisInfo){.range =
                                               &(GuiAxisRange){
                                                   .min = -0.1,
                                                   .max = +1.1,
                                                   .locked = true,
                                               }},
    };

    if (guiPlotBegin("plot", &plot))
    {
        guiPlotLineF64("Y", samples[0].elem, samples[0].elem + 1, CF_ARRAY_SIZE(samples), 0, 3);
        guiPlotLineF64("Z", samples[0].elem, samples[0].elem + 2, CF_ARRAY_SIZE(samples), 0, 3);
        guiPlotEnd();
    }
    guiEnd();
}

APP_API APP_CREATE_FN(appCreate)
{
    CF_UNUSED(cmd_line);

    // NOTE (Matteo): Memory comes cleared to 0
    AppState *app = memAllocStruct(plat->heap, AppState);

    app->plat = plat;
    app->alloc = plat->heap;
    app->windows.stats = true;
    app->pwm.duty_cyle = 0.5;
    app->pwm.frequency = 1.0 / 10;

    appLoad(app);

    return app;
}

APP_API APP_FN(appDestroy)
{
    appUnload(app);
    memFreeStruct(app->alloc, app);
}

APP_API APP_FN(appLoad)
{
    CF_ASSERT_NOT_NULL(app);
    CF_ASSERT_NOT_NULL(app->plat);

    // Init Dear Imgui
    guiSetContext(app->plat->gui);

    // Init OpenGl
    glApiSet(app->plat->gl);

    // glEnable(GL_FRAMEBUFFER_SRGB);
    glDisable(GL_FRAMEBUFFER_SRGB);
}

APP_API APP_FN(appUnload)
{
    CF_ASSERT_NOT_NULL(app);
}

APP_API APP_UPDATE_FN(appUpdate)
{
    Platform *plat = state->plat;

    //=== Menu bar ===//

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

    //=== Main window ===//

    guiDockSpace(GuiDockStyle_Transparent);
    pwmWindow(state);

    //=== Service windows ===//

    if (state->windows.metrics) guiMetricsWindow(&state->windows.metrics);

    if (state->windows.fonts)
    {
        guiBegin("Font Options", &state->windows.fonts);
        io->rebuild_fonts = guiFontOptionsEdit(io->font_opts);
        guiEnd();
    }

    if (state->windows.stats)
    {
        F64 framerate = (F64)guiGetFramerate();
        GuiMemory gui_mem = guiMemoryInfo();

        guiBegin("Application stats", &state->windows.stats);
        guiText("Average %.3f ms/frame (%.1f FPS)", 1000.0 / framerate, framerate);

        guiSeparator();
        guiText("Virtual memory: Reserved %.3fkb - Committed %.3fkb",
                (F64)plat->reserved_size / 1024, (F64)plat->committed_size / 1024);
        guiText("Heap memory   : Allocated %.3fkb in %zu blocks", (F64)plat->heap_size / 1024,
                plat->heap_blocks);
        guiText("GUI memory    : Allocated %.3fkb in %zu blocks", (F64)gui_mem.size / 1024,
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

    io->back_color = SRGB32_SOLID(115, 140, 153);
}
