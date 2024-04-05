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
    bool gui_demo;
    bool plot_demo;
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

    while (app->log.write_pos < app->log.size)
    {
        cfLogAppendC(&app->log, "Filling log buffer\n");
    }

    cfLogAppendC(&app->log, "Filling log buffer\n");

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

    if (app->srgb_framebuffer)
    {
        glEnable(GL_FRAMEBUFFER_SRGB);
    }
    else
    {
        glDisable(GL_FRAMEBUFFER_SRGB);
    }
}

APP_API
APP_FN(appUnload)
{
    CF_ASSERT_NOT_NULL(app);
    g_log = NULL;
}

//------------------------------------------------------------------------------

typedef struct QueryResult
{
    float x0, x1, distance;
} QueryResult;

#define FLT_MAX_ITER FLT_MANT_DIG - FLT_MIN_EXP
#define DBL_MAX_ITER DBL_MANT_DIG - DBL_MIN_EXP

static float
RobustLength(float x, float y)
{
    float abs_x = mAbs(x);
    float abs_y = mAbs(y);

    if (abs_x > abs_y) return abs_x * mSqrt(1 + mSquare(y / x));
    if (abs_x < abs_y) return abs_y * mSqrt(1 + mSquare(x / y));

    return mSqrt(x * x + y * y);
}

static float
GetRoot(float r0, float z0, float z1, float g)
{
    float n0 = r0 * z0;

    float s0 = z1 - 1;
    float s1 = (g < 0 ? 0 : RobustLength(n0, z1) - 1);
    float s = 0;

    CF_STATIC_ASSERT(FLT_MAX_ITER > 0, "Wrong number of iterations");

    U32 iter = 0;

    for (;;)
    {
        s = (s0 + s1) / 2;

        if (s == s0 || s == s1) break;

        float ratio0 = n0 / (s + r0);
        float ratio1 = z1 / (s + 1);

        g = mSquare(ratio0) + mSquare(ratio1) - 1;

        if (g > 0)
        {
            s0 = s;
        }
        else if (g < 0)
        {
            s1 = s;
        }
        else
        {
            break;
        }

        if (++iter == FLT_MAX_ITER)
        {
            appLog("GetRoot reached max # of iterations: %u\n", iter);
            break;
        }
    }

    appLog("GetRoot # of iterations: %u\n", iter);

    return s;
}

Vec3f
DistancePointEllipse(float e0, float e1, Vec2f p)
{
    Vec3f result = {0};

    Vec2f p_ori = p;

    p.x = mAbs(p.x);
    p.y = mAbs(p.y);

    if (p.y > 0)
    {
        if (p.x > 0)
        {
            float z0 = p.x / e0;
            float z1 = p.y / e1;
            float g = mSquare(z0) + mSquare(z1) - 1;

            if (g != 0)
            {
                float r0 = mSquare(e0 / e1);
                float sbar = GetRoot(r0, z0, z1, g);
                result.x = r0 * p.x / (sbar + r0);
                result.y = p.y / (sbar + 1);
                result.z = mSqrt(mSquare(result.x - p.x) + mSquare(result.y - p.y));
            }
            else
            {
                result.x = p.x;
                result.y = p.y;
                result.z = 0;
            }
        }
        else // p.x == 0
        {
            result.x = 0;
            result.y = e1;
            result.z = mAbs(p.y - e1);
        }
    }
    else //  p.y == 0
    {
        float numer0 = e0 * p.x;
        float denom0 = mSquare(e0) - mSquare(e1);

        if (numer0 < denom0)
        {
            float xde0 = numer0 / denom0;
            result.x = e0 * xde0;
            result.y = e1 * mSqrt(1 - xde0 * xde0);
            result.z = mSqrt(mSquare(result.x - p.x) + mSquare(result.y - p.y));
        }
        else
        {
            result.x = e0;
            result.y = 0;
            result.z = mAbs(p.x - e0);
        }
    }

    result.x = mCopySign(result.x, p_ori.x);
    result.y = mCopySign(result.y, p_ori.y);

    return result;
}

static Vec2f
rotateFwd(Vec2f p, float cos, float sin)
{
    Vec2f r = {0};
    r.x = p.x * cos - p.y * sin;
    r.y = p.x * sin + p.y * cos;
    return r;
}

static Vec2f
rotateBwd(Vec2f p, float cos, float sin)
{
    Vec2f r = {0};
    r.x = p.x * cos + p.y * sin;
    r.y = p.y * cos - p.x * sin;
    return r;
}

static float
normalize(float value, float range, float offset)
{
    return value - range * mFloor((value + offset) / range);
}

void
fxEllipse(GuiCanvas *canvas, Vec4f mouse_data, double time)
{
    static float const pi2 = 2 * M_PI_F;

    // Center of the view
    Vec2f const center = {.x = (canvas->p0.x + canvas->p1.x) / 2, //
                          .y = (canvas->p0.y + canvas->p1.y) / 2};

    // Rotation of the ellipse
    float const rot = normalize(0.1f * pi2 * (float)time, M_PI_F, 0);
    float const cosw = mCos(rot);
    float const sinw = mSin(rot);

    // Draw the ellipse as a series of segments
    Vec2f points[1024] = {0};
    float const rad_step = 2 * pi2 / (CF_ARRAY_SIZE(points) - 1);

    float const extent = cfMin(canvas->size.x, canvas->size.y);
    float const a = 3 * extent / 8;
    float const b = 3 * a / 5;
    for (Size i = 0; i < CF_ARRAY_SIZE(points); ++i)
    {
        float rad = (float)i * rad_step;
        float cost = mCos(rad);
        float sint = mSin(rad);

        points[i].x = center.x + a * cost * cosw - b * sint * sinw;
        points[i].y = center.y + a * cost * sinw + b * sint * cosw;
    }

    canvas->stroke_color = SRGB32_YELLOW;
    guiCanvasDrawPolyline(canvas, points, CF_ARRAY_SIZE(points));

    // Draw mouse position and nearest point on the ellipse
    Vec2f query_pt = vecSub(mouse_data.xy, center);
    query_pt = rotateBwd(query_pt, cosw, sinw);

    Vec2f query_res = DistancePointEllipse(a, b, query_pt).xy;
    query_res = rotateFwd(query_res, cosw, sinw);

    canvas->stroke_color = canvas->fill_color = SRGB32_ORANGE_RED;

    guiCanvasFillCircle(canvas, mouse_data.xy, 5.0f);

    guiCanvasFillCircle(canvas, vecAdd(query_res, center), 5.0f);

    guiCanvasDrawLine(canvas, mouse_data.xy, vecAdd(query_res, center));

    // Draw intersection on the Y axis
    float sinw2 = sinw * sinw;
    float cosw2 = cosw * cosw;
    Vec2f itx = {.x = 0, .y = a * b * mRsqrt(a * a * cosw2 + b * b * sinw2)};

    canvas->stroke_color = SRGB32_CYAN;
    guiCanvasDrawLine(canvas,                                    //
                      (Vec2f){.x = center.x, .y = canvas->p0.y}, //
                      (Vec2f){.x = center.x, .y = canvas->p1.y});

    canvas->stroke_color = SRGB32_ORANGE_RED;
    guiCanvasDrawLine(canvas, center, (Vec2f){.x = center.x + itx.x, .y = center.y + itx.y});

    // Place a circle on the ellipse
    float const circ_rad = b / 3;
    Vec2f circ_center = {.x = 0, .y = -canvas->size.y * 2};

    query_res = DistancePointEllipse(a, b, rotateBwd(circ_center, cosw, sinw)).xy;
    query_res = rotateFwd(query_res, cosw, sinw);

    Vec2f circ_pt = vecMul(vecNormalize(vecSub(query_res, circ_center)), circ_rad);
    Vec2f delta = vecSub2f(query_res, circ_pt);

    circ_center = vecAdd2f(circ_center, delta);

    canvas->stroke_color = SRGB32_YELLOW;
    guiCanvasDrawCircle(canvas, vecAdd(circ_center, center), circ_rad);

    canvas->fill_color = SRGB32_FUCHSIA;
    guiCanvasFillCircle(canvas, vecAdd(query_res, center), 5.0f);
}

void
fxSine(GuiCanvas *canvas, Vec4f mouse_data, double time)
{
    // 1 Hz sinusoid, YAY!

    static Vec2f points[1024] = {0};

    static float const pi2 = 2 * M_PI_F;
    static float const rad_step = 4 * M_PI_F / (CF_ARRAY_SIZE(points) - 1);

    float const amp = cfMin(canvas->size.x, canvas->size.y) / 4;

    float const x_offset = 2 * amp;
    float const x_space = canvas->size.x - x_offset;
    float const x_step = x_space / (CF_ARRAY_SIZE(points) - 1);
    float const y_offset = canvas->size.y / 2;

    float const phase = pi2 * (float)time;

    Vec2f const center = {.x = canvas->p0.x + amp, .y = canvas->p0.y + y_offset};
    Vec2f polar = {0};

    for (Size i = 0; i < CF_ARRAY_SIZE(points); ++i)
    {
        float rad = (float)i * rad_step + phase;
        float sin = amp * mSin(rad);
        float cos = amp * mCos(rad);

        points[i].x = canvas->p0.x + x_offset + x_step * (float)i;
        points[i].y = canvas->p0.y + y_offset + sin;

        polar.x = center.x + cos;
        polar.y = center.y + sin;
    }

    canvas->stroke_color = SRGB32_YELLOW;
    guiCanvasDrawCircle(canvas, center, amp);
    guiCanvasDrawLine(canvas, center, polar);
    guiCanvasDrawLine(canvas, polar, points[0]);
    guiCanvasDrawPolyline(canvas, points, CF_ARRAY_SIZE(points));

    Vec2f p0 = canvas->p0;
    Vec2f p1 = canvas->p1;

    canvas->stroke_color = SRGB32_PURPLE;
    guiCanvasDrawLine(canvas, //
                      (Vec2f){.x = p0.x, .y = p0.y + y_offset},
                      (Vec2f){.x = p1.x, .y = p0.y + y_offset});

    guiCanvasDrawLine(canvas,                              //
                      (Vec2f){.x = p0.x + amp, .y = p0.y}, //
                      (Vec2f){.x = p0.x + amp, .y = p1.y});

    guiCanvasDrawLine(canvas, //
                      (Vec2f){.x = p0.x + 2 * amp, .y = p0.y},
                      (Vec2f){.x = p0.x + 2 * amp, .y = p1.y});

    canvas->fill_color = SRGB32_ORANGE_RED;
    guiCanvasFillCircle(canvas, mouse_data.xy, 5.0f);
}

static void
fxDraw(GuiCanvas *canvas, Vec4f mouse_data, double time)
{
    canvas->stroke_color = SRGB32_RED;
    guiCanvasDrawRect(canvas, canvas->p0, canvas->p1);

    Char8 buffer[1024];
    strPrint(buffer, CF_ARRAY_SIZE(buffer), "%f", time);
    guiCanvasDrawText(canvas, strFromCstr(buffer), canvas->p0, SRGB32_RED);

    // fxSine(canvas, mouse_data, time);
    fxEllipse(canvas, mouse_data, time);
}

static void
fxWindow(void)
{
    guiSetNextWindowSize((Vec2f){{320, 180}}, GuiCond_Once);
    guiBegin("FX", NULL);

    GuiCanvas canvas = {0};
    guiCanvasBegin(&canvas);

    Vec4f mouse_data;
    mouse_data.xy = guiGetMousePos();
    mouse_data.z = guiGetMouseDownDuration(GuiMouseButton_Left);
    mouse_data.w = guiGetMouseDownDuration(GuiMouseButton_Right);

    Duration now = clockElapsed(g_clock);
    fxDraw(&canvas, mouse_data, timeGetSeconds(now));

    guiCanvasEnd(&canvas);

    guiEnd();
}

static void
fxDrawArc(GuiCanvas *canvas, Vec2f center, Vec2f p0, Vec2f p1, float radius, Srgb32 color)
{
    Vec2f points[1024];

    float dx = p1.x - p0.x;
    float dy = p1.y - p0.y;
    float chord = mSqrt(dx * dx + dy * dy);

    float span = 2 * mAsin(chord / (2 * mAbs(radius)));

    float step = mCopySign(span / (CF_ARRAY_SIZE(points) - 1), radius);
    float cos = mCos(step);
    float sin = mSin(step);

    Vec2f v0 = vecSub(p0, center);

    for (Size i = 0; i < CF_ARRAY_SIZE(points); ++i)
    {
        points[i] = vecAdd(v0, center);
        // points[i].x = center.x + v0.x;
        // points[i].y = center.y + v0.y;
        v0 = rotateFwd(v0, cos, sin);
    }

    Srgb32 prev_color = canvas->stroke_color;
    canvas->stroke_color = color;
    guiCanvasDrawPolyline(canvas, points, CF_ARRAY_SIZE(points));
    canvas->stroke_color = prev_color;
}

void
fxTangentCircles(void)
{
#define deg2rad (M_PI_F / 180.0f)

    static float const cutter_start = deg2rad * 270.0f;
    static float const cutter_end = deg2rad * 70.0f;

    static float lens_radius_parm = 50;
    static float depth = 1;

    float crib = 60;
    float cutter_radius = 1;

    guiSetNextWindowSize((Vec2f){.x = 320, .y = 180}, GuiCond_Once);
    guiBegin("Tangent circles", NULL);

    guiSliderF32("Radius", &lens_radius_parm, 50, 1000);
    guiSliderF32("Depth", &depth, 0, 5);
    guiSeparator();

    GuiCanvas canvas = {0};
    guiCanvasBegin(&canvas);

    float scale = canvas.size.y / (crib / 2 + 5.0f);

    float lens_radius = lens_radius_parm * scale;
    crib *= scale;
    cutter_radius *= scale;

    Vec2f lens_center = {.x = canvas.p0.x + canvas.size.x / 2 + lens_radius, //
                         .y = canvas.p1.y};

    float crib_x = mSqrt(lens_radius * lens_radius - crib * crib / 4);
    Vec2f lens_start = {.x = lens_center.x - crib_x, .y = lens_center.y + crib / 2};
    Vec2f lens_end = {.x = lens_center.x - crib_x, .y = lens_center.y - crib / 2};
    float lens_angle = mAtan2(crib / 2, crib_x);

    fxDrawArc(&canvas, lens_center, lens_start, lens_end, lens_radius, SRGB32_FUCHSIA);

    Vec2f cutter_center = {.x = lens_end.x - mCos(lens_angle) * cutter_radius,
                           .y = lens_end.y - mSin(lens_angle) * cutter_radius};

    float delta_angle = lens_angle - cutter_end;
    float dcos = mCos(delta_angle);
    float dsin = mSin(delta_angle);

    Vec2f cutter_start_p = {.x = cutter_radius * mCos(cutter_start),
                            .y = cutter_radius * mSin(cutter_start)};

    Vec2f cutter_end_p = {.x = cutter_radius * mCos(cutter_end),
                          .y = cutter_radius * mSin(cutter_end)};

    cutter_start_p = vecAdd(cutter_center, rotateFwd(cutter_start_p, dcos, dsin));
    cutter_end_p = vecAdd(cutter_center, rotateFwd(cutter_end_p, dcos, dsin));

    fxDrawArc(&canvas, cutter_center, cutter_start_p, cutter_end_p, cutter_radius, SRGB32_YELLOW);

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

static void
guiFrameratePlot()
{
    // TODO (Matteo): Is there a better way to implement a scrolling buffer?
    static Vec2d samples[256] = {0};
    static Size sample_count = 0;
    static const Size mask = CF_ARRAY_SIZE(samples) - 1;

    Size count, offset;
    Size index = index = sample_count & mask;

    if (index != sample_count)
    {
        offset = (index + 1) & mask;
        count = CF_ARRAY_SIZE(samples);
    }
    else
    {
        offset = 0;
        count = sample_count + 1;
    }

    double framerate = (double)guiGetFramerate();
    double window = (double)CF_ARRAY_SIZE(samples) / framerate;
    double time = 1.0 / framerate;
    if (sample_count) time += samples[(sample_count - 1) & mask].x;

    samples[index].x = time;
    samples[index].y = framerate;
    ++sample_count;

    GuiPlotSetup plot = {
        .legend = &(GuiPlotLegend){.location = GuiLocation_S, .outside = true},
        .info[GuiAxis_Y1] = &(GuiAxisInfo){.label = "Hz", .autofit = true},
        .info[GuiAxis_X1] = &(GuiAxisInfo){.range =
                                               &(GuiAxisRange){
                                                   .min = time - window,
                                                   .max = time,
                                                   .locked = true,
                                               }},
    };

    if (guiPlotBegin("plot", &plot))
    {
        guiPlotLineF64("Framerate", samples[0].elem, samples[0].elem + 1, count, offset, 2);
        guiPlotEnd();
    }
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
            guiSeparator();
            guiMenuItem("Gui demo", &state->windows.gui_demo);
            guiMenuItem("Plot demo", &state->windows.plot_demo);
            guiEndMenu();
        }

        guiEndMainMenuBar();
    }

    guiDockSpace(GuiDockStyle_Transparent);

    if (state->windows.gui_demo) guiDemoWindow(&state->windows.gui_demo);
    if (state->windows.plot_demo) guiPlotDemoWindow(&state->windows.plot_demo);

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

    guiBegin("Test", NULL);
    {
        static float f = 0;

        Clock *clock = &plat->clock;
        Duration t = clockElapsed(clock);
        CalendarTime now = localTime(systemTime());

        guiCheckbox("Gui demo", &state->windows.gui_demo);
        guiSameLine();
        guiCheckbox("Plot demo", &state->windows.plot_demo);
        guiSliderF32("float", &f, 0.0f, 1.0f);
        guiColorEdit("clear color", &state->clear_color);
        guiSeparator();
        guiThemeSelector("Theme");
        guiCheckbox("Continuous update", &io->continuous_update);
        guiSameLine();
        guiCheckbox("Fullscreen", &io->fullscreen);
        guiSameLine();
        if (guiCheckbox("Srgb", &state->srgb_framebuffer))
        {
            glApiToggle(GL_FRAMEBUFFER_SRGB, state->srgb_framebuffer);
            guiGammaCorrection(state->srgb_framebuffer);
        }
        guiSeparator();
        guiText("%04d/%02d/%02d %02d:%02d:%02d.%03d", now.year, now.month, now.day, now.hour,
                now.minute, now.second, now.milliseconds);
        guiClock(t);
        guiSeparator();

        guiFrameratePlot();
        guiSeparator();

        if (timeIsGe(timeSub(t, state->log_time), timeDurationMs(1000)))
        {
            state->log_time = t;
            cfLogAppendF(&state->log, "One second passed\n");
        }

        guiLogBox(&state->log, false);
    }
    guiEnd();

    fxWindow();
    fxTangentCircles();

    io->back_color = state->clear_color;
}
