#include "app.h"
#include "platform.h"

#include "foundation/colors.h"
#include "foundation/log.h"
#include "foundation/math.inl"
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

static CfLog *g_log = NULL;
static CfTimeApi *g_time = NULL;

#define appLog(...) (g_log ? (cfLogAppendF(g_log, __VA_ARGS__), 1) : 0)

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

    appLoad(app);

    return app;
}

APP_API APP_PROC(appDestroy)
{
    appUnload(app);
    cfLogDestroy(&app->log, app->plat->vm);
    memFree(app->alloc, app, sizeof(*app));
}

APP_API APP_PROC(appLoad)
{
    CF_ASSERT_NOT_NULL(app);
    CF_ASSERT_NOT_NULL(app->plat);

    // Init globals
    g_log = &app->log;
    g_time = app->plat->time;

    // Init Dear Imgui
    guiInit(app->plat->gui);

    // Init OpenGl
    gloadInit(app->plat->gl);
}

APP_API APP_PROC(appUnload)
{
    CF_ASSERT_NOT_NULL(app);
    g_log = NULL;
}

//------------------------------------------------------------------------------

typedef struct QueryResult
{
    F32 x0, x1, distance;
} QueryResult;

#define MAX_ITER(Type) F_DIGITS(Type) - F_MIN_EXP(Type)

static F32
RobustLength(F32 x, F32 y)
{
    F32 abs_x = mAbs(x);
    F32 abs_y = mAbs(y);

    if (abs_x > abs_y) return abs_x * mSqrt(1 + mSquare(y / x));
    if (abs_x < abs_y) return abs_y * mSqrt(1 + mSquare(x / y));

    return mSqrt(x * x + y * y);
}

static F32
GetRoot(F32 r0, F32 z0, F32 z1, F32 g)
{
    F32 n0 = r0 * z0;

    F32 s0 = z1 - 1;
    F32 s1 = (g < 0 ? 0 : RobustLength(n0, z1) - 1);
    F32 s = 0;

    static_assert(MAX_ITER(F32) > 0, "Wrong number of iterations");

    U32 iter = 0;

    for (;;)
    {
        s = (s0 + s1) / 2;

        if (s == s0 || s == s1) break;

        F32 ratio0 = n0 / (s + r0);
        F32 ratio1 = z1 / (s + 1);

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

        if (++iter == MAX_ITER(F32))
        {
            appLog("GetRoot reached max # of iterations: %u\n", iter);
            break;
        }
    }

    appLog("GetRoot # of iterations: %u\n", iter);

    return s;
}

Vec3
DistancePointEllipse(F32 e0, F32 e1, Vec2 p)
{
    Vec3 result = {0};

    Vec2 p_ori = p;

    p.x = mAbs(p.x);
    p.y = mAbs(p.y);

    if (p.y > 0)
    {
        if (p.x > 0)
        {
            F32 z0 = p.x / e0;
            F32 z1 = p.y / e1;
            F32 g = mSquare(z0) + mSquare(z1) - 1;

            if (g != 0)
            {
                F32 r0 = mSquare(e0 / e1);
                F32 sbar = GetRoot(r0, z0, z1, g);
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
        F32 numer0 = e0 * p.x;
        F32 denom0 = mSquare(e0) - mSquare(e1);

        if (numer0 < denom0)
        {
            F32 xde0 = numer0 / denom0;
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

static Vec2
rotateFwd(Vec2 p, F32 cos, F32 sin)
{
    Vec2 r = {0};
    r.x = p.x * cos - p.y * sin;
    r.y = p.x * sin + p.y * cos;
    return r;
}

static Vec2
rotateBwd(Vec2 p, F32 cos, F32 sin)
{
    Vec2 r = {0};
    r.x = p.x * cos + p.y * sin;
    r.y = p.y * cos - p.x * sin;
    return r;
}

static F32
normalize(F32 value, F32 range, F32 offset)
{
    return value - range * mFloor((value + offset) / range);
}

void
fxEllipse(GuiCanvas *canvas, Vec4 mouse_data, F64 time)
{
    static F32 const pi2 = 2 * M_PI32;

    // Center of the view
    Vec2 const center = {.x = (canvas->p0.x + canvas->p1.x) / 2, //
                         .y = (canvas->p0.y + canvas->p1.y) / 2};

    // Rotation of the ellipse
    F32 const rot = normalize(0.1f * pi2 * (F32)time, M_PI32, 0);
    F32 const cosw = mCos(rot);
    F32 const sinw = mSin(rot);

    // Draw the ellipse as a series of segments
    Vec2 points[1024] = {0};
    F32 const rad_step = 2 * pi2 / (CF_ARRAY_SIZE(points) - 1);

    F32 const extent = cfMin(canvas->size.x, canvas->size.y);
    F32 const a = 3 * extent / 8;
    F32 const b = 3 * a / 5;
    for (Usize i = 0; i < CF_ARRAY_SIZE(points); ++i)
    {
        F32 rad = (F32)i * rad_step;
        F32 cost = mCos(rad);
        F32 sint = mSin(rad);

        points[i].x = center.x + a * cost * cosw - b * sint * sinw;
        points[i].y = center.y + a * cost * sinw + b * sint * cosw;
    }

    canvas->stroke_color = RGBA32_YELLOW;
    guiCanvasDrawPolyline(canvas, points, CF_ARRAY_SIZE(points));

    // Draw mouse position and nearest point on the ellipse
    Vec2 query_pt = {.x = mouse_data.x - center.x, //
                     .y = mouse_data.y - center.y};
    query_pt = rotateBwd(query_pt, cosw, sinw);

    Vec2 query_res = DistancePointEllipse(a, b, query_pt).xy;
    query_res = rotateFwd(query_res, cosw, sinw);

    canvas->stroke_color = canvas->fill_color = RGBA32_ORANGE_RED;

    guiCanvasFillCircle(canvas, mouse_data.xyz.xy, 5.0f);

    guiCanvasFillCircle(canvas, vecAdd(query_res, center), 5.0f);

    guiCanvasDrawLine(canvas, mouse_data.xyz.xy, vecAdd(query_res, center));

    // Draw intersection on the Y axis
    F32 sinw2 = sinw * sinw;
    F32 cosw2 = cosw * cosw;
    Vec2 itx = {.x = 0, .y = a * b * mRsqrt(a * a * cosw2 + b * b * sinw2)};

    canvas->stroke_color = RGBA32_CYAN;
    guiCanvasDrawLine(canvas,                                   //
                      (Vec2){.x = center.x, .y = canvas->p0.y}, //
                      (Vec2){.x = center.x, .y = canvas->p1.y});

    canvas->stroke_color = RGBA32_ORANGE_RED;
    guiCanvasDrawLine(canvas, center, (Vec2){.x = center.x + itx.x, .y = center.y + itx.y});

    // Place a circle on the ellipse
    F32 const circ_rad = b / 3;
    Vec2 circ_center = {.x = 0, .y = -canvas->size.y * 2};

    query_res = DistancePointEllipse(a, b, rotateBwd(circ_center, cosw, sinw)).xy;
    query_res = rotateFwd(query_res, cosw, sinw);

    Vec2 circ_pt = vecMul(vecNormalize(vecSub(query_res, circ_center)), circ_rad);
    Vec2 delta = vecSub2(query_res, circ_pt);

    circ_center = vecAdd2(circ_center, delta);

    canvas->stroke_color = RGBA32_YELLOW;
    guiCanvasDrawCircle(canvas, vecAdd(circ_center, center), circ_rad);

    canvas->fill_color = RGBA32_FUCHSIA;
    guiCanvasFillCircle(canvas, vecAdd(query_res, center), 5.0f);
}

void
fxSine(GuiCanvas *canvas, Vec4 mouse_data, F64 time)
{
    // 1 Hz sinusoid, YAY!

    static Vec2 points[1024] = {0};

    static F32 const pi2 = 2 * M_PI32;
    static F32 const rad_step = 4 * M_PI32 / (CF_ARRAY_SIZE(points) - 1);

    F32 const amp = cfMin(canvas->size.x, canvas->size.y) / 4;

    F32 const x_offset = 2 * amp;
    F32 const x_space = canvas->size.x - x_offset;
    F32 const x_step = x_space / (CF_ARRAY_SIZE(points) - 1);
    F32 const y_offset = canvas->size.y / 2;

    F32 const phase = pi2 * (F32)time;

    Vec2 const center = {.x = canvas->p0.x + amp, .y = canvas->p0.y + y_offset};
    Vec2 polar = {0};

    for (Usize i = 0; i < CF_ARRAY_SIZE(points); ++i)
    {
        F32 rad = (F32)i * rad_step + phase;
        F32 sin = amp * mSin(rad);
        F32 cos = amp * mCos(rad);

        points[i].x = canvas->p0.x + x_offset + x_step * (F32)i;
        points[i].y = canvas->p0.y + y_offset + sin;

        polar.x = center.x + cos;
        polar.y = center.y + sin;
    }

    canvas->stroke_color = RGBA32_YELLOW;
    guiCanvasDrawCircle(canvas, center, amp);
    guiCanvasDrawLine(canvas, center, polar);
    guiCanvasDrawLine(canvas, polar, points[0]);
    guiCanvasDrawPolyline(canvas, points, CF_ARRAY_SIZE(points));

    Vec2 p0 = canvas->p0;
    Vec2 p1 = canvas->p1;

    canvas->stroke_color = RGBA32_PURPLE;
    guiCanvasDrawLine(canvas, //
                      (Vec2){.x = p0.x, .y = p0.y + y_offset},
                      (Vec2){.x = p1.x, .y = p0.y + y_offset});

    guiCanvasDrawLine(canvas,                             //
                      (Vec2){.x = p0.x + amp, .y = p0.y}, //
                      (Vec2){.x = p0.x + amp, .y = p1.y});

    guiCanvasDrawLine(canvas, //
                      (Vec2){.x = p0.x + 2 * amp, .y = p0.y},
                      (Vec2){.x = p0.x + 2 * amp, .y = p1.y});

    canvas->fill_color = RGBA32_ORANGE_RED;
    guiCanvasFillCircle(canvas, mouse_data.xyz.xy, 5.0f);
}

static void
fxDraw(GuiCanvas *canvas, Vec4 mouse_data, F64 time)
{
    guiCanvasDrawRect(canvas, canvas->p0, canvas->p1);

    Char8 buffer[1024];
    strPrintf(buffer, CF_ARRAY_SIZE(buffer), "%f", time);
    guiCanvasDrawText(canvas, strFromCstr(buffer), canvas->p0, RGBA32_RED);

    // fxSine(canvas, mouse_data, time);
    fxEllipse(canvas, mouse_data, time);
}

static void
fxWindow(void)
{
    guiSetNextWindowSize((Vec2){{320, 180}}, GuiCond_Once);
    guiBegin("FX", NULL);

    GuiCanvas canvas = {0};
    guiCanvasBegin(&canvas);

    Vec4 mouse_data;
    mouse_data.xyz.xy = guiGetMousePos();
    mouse_data.z = guiGetMouseDownDuration(GuiMouseButton_Left);
    mouse_data.w = guiGetMouseDownDuration(GuiMouseButton_Right);

    Duration now = g_time->clock();
    fxDraw(&canvas, mouse_data, timeGetSeconds(now));

    guiCanvasEnd(&canvas);

    guiEnd();
}

static void
fxDrawArc(GuiCanvas *canvas, Vec2 center, Vec2 p0, Vec2 p1, F32 radius, Rgba32 color)
{
    Vec2 points[1024];

    F32 dx = p1.x - p0.x;
    F32 dy = p1.y - p0.y;
    F32 chord = mSqrt(dx * dx + dy * dy);

    F32 span = 2 * mAsin(chord / (2 * mAbs(radius)));

    F32 step = mCopySign(span / (CF_ARRAY_SIZE(points) - 1), radius);
    F32 cos = mCos(step);
    F32 sin = mSin(step);

    Vec2 v0 = vecSub(p0, center);

    for (Usize i = 0; i < CF_ARRAY_SIZE(points); ++i)
    {
        points[i] = vecAdd(v0, center);
        // points[i].x = center.x + v0.x;
        // points[i].y = center.y + v0.y;
        v0 = rotateFwd(v0, cos, sin);
    }

    Rgba32 prev_color = canvas->stroke_color;
    canvas->stroke_color = color;
    guiCanvasDrawPolyline(canvas, points, CF_ARRAY_SIZE(points));
    canvas->stroke_color = prev_color;
}

void
fxTangentCircles(void)
{
#define deg2rad (M_PI32 / 180.0f)

    static F32 const cutter_start = deg2rad * 270.0f;
    static F32 const cutter_end = deg2rad * 70.0f;

    static F32 lens_radius_parm = 50;
    static F32 depth = 1;

    F32 crib = 60;
    F32 cutter_radius = 1;

    guiSetNextWindowSize((Vec2){.x = 320, .y = 180}, GuiCond_Once);
    guiBegin("Tangent circles", NULL);

    guiSlider("Radius", &lens_radius_parm, 50, 1000);
    guiSlider("Depth", &depth, 0, 5);
    guiSeparator();

    GuiCanvas canvas = {0};
    guiCanvasBegin(&canvas);

    F32 scale = canvas.size.y / (crib / 2 + 5.0f);

    F32 lens_radius = lens_radius_parm * scale;
    crib *= scale;
    cutter_radius *= scale;

    Vec2 lens_center = {.x = canvas.p0.x + canvas.size.x / 2 + lens_radius, //
                        .y = canvas.p1.y};

    F32 crib_x = mSqrt(lens_radius * lens_radius - crib * crib / 4);
    Vec2 lens_start = {.x = lens_center.x - crib_x, .y = lens_center.y + crib / 2};
    Vec2 lens_end = {.x = lens_center.x - crib_x, .y = lens_center.y - crib / 2};
    F32 lens_angle = mAtan2(crib / 2, crib_x);

    fxDrawArc(&canvas, lens_center, lens_start, lens_end, lens_radius, RGBA32_FUCHSIA);

    Vec2 cutter_center = {.x = lens_end.x - mCos(lens_angle) * cutter_radius,
                          .y = lens_end.y - mSin(lens_angle) * cutter_radius};

    F32 delta_angle = lens_angle - cutter_end;
    F32 dcos = mCos(delta_angle);
    F32 dsin = mSin(delta_angle);

    Vec2 cutter_start_p = {.x = cutter_radius * mCos(cutter_start),
                           .y = cutter_radius * mSin(cutter_start)};

    Vec2 cutter_end_p = {.x = cutter_radius * mCos(cutter_end),
                         .y = cutter_radius * mSin(cutter_end)};

    cutter_start_p = vecAdd(cutter_center, rotateFwd(cutter_start_p, dcos, dsin));
    cutter_end_p = vecAdd(cutter_center, rotateFwd(cutter_end_p, dcos, dsin));

    fxDrawArc(&canvas, cutter_center, cutter_start_p, cutter_end_p, cutter_radius, RGBA32_YELLOW);

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

APP_API APP_UPDATE_PROC(appUpdate)
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
            guiMenuItem("Demo window", &state->windows.demo);
            guiEndMenu();
        }

        guiEndMainMenuBar();
    }

    if (state->windows.demo) guiDemoWindow(&state->windows.demo);

    if (state->windows.fonts)
    {
        guiBegin("Font Options", &state->windows.fonts);
        io->rebuild_fonts = guiFontOptionsEdit(io->font_opts);
        guiEnd();
    }

    if (state->windows.stats)
    {
        F64 framerate = (F64)guiGetFramerate();

        guiBegin("Application stats stats", &state->windows.stats);
        guiText("Average %.3f ms/frame (%.1f FPS)", 1000.0 / framerate, framerate);
        guiSeparator();
        guiText("Allocated %.3fkb in %zu blocks", (F64)plat->heap_size / 1024, plat->heap_blocks);
        guiText("Virtual memory reserved %.3fkb - committed %.3fkb",
                (F64)plat->reserved_size / 1024, (F64)plat->committed_size / 1024);
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
        static F32 f = 0;

        CfTimeApi *time = plat->time;
        Duration t = time->clock();
        CalendarTime now = time->localTime(time->systemTime());

        guiCheckbox("Demo Window", &state->windows.demo);
        guiSlider("float", &f, 0.0f, 1.0f);
        guiColorEdit("clear color", &state->clear_color);
        guiSeparator();
        guiThemeSelector("Theme");
        guiCheckbox("Continuous update", &io->continuous_update);
        guiSameLine();
        guiCheckbox("Fullscreen", &io->fullscreen);
        guiSeparator();
        guiText("%04d/%02d/%02d %02d:%02d:%02d.%03d", now.year, now.month, now.day, now.hour,
                now.minute, now.second, now.milliseconds);
        guiClock(t);
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
