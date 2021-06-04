#ifndef FOUNDATION_COLOR_H

#include "common.h"
#include "util.h"

#define RGBA32_R_SHIFT 0
#define RGBA32_G_SHIFT 8
#define RGBA32_B_SHIFT 16
#define RGBA32_A_SHIFT 24
#define RGBA32_A_MASK 0xFF000000

#define RGBA32(R, G, B, A)                                               \
    (((Rgba32)(A) << RGBA32_A_SHIFT) | ((Rgba32)(B) << RGBA32_B_SHIFT) | \
     ((Rgba32)(G) << RGBA32_G_SHIFT) | ((Rgba32)(R) << RGBA32_R_SHIFT))

#define RGBA32_WHITE RGBA32(0xFF, 0xFF, 0xFF, 0xFF) // Opaque white = 0xFFFFFFFF
#define RGBA32_BLACK RGBA32(0x00, 0x00, 0x00, 0xFF) // Opaque black
#define RGBA32_TRANS RGBA32(0x00, 0x00, 0x00, 0x00) // Transparent black = 0x00000000

#define RGBA32_R(col) ((col >> RGBA32_R_SHIFT) & 0xFF)
#define RGBA32_G(col) ((col >> RGBA32_G_SHIFT) & 0xFF)
#define RGBA32_B(col) ((col >> RGBA32_B_SHIFT) & 0xFF)
#define RGBA32_A(col) ((col >> RGBA32_A_SHIFT) & 0xFF)

static inline Rgba
rgbaUnpack32(Rgba32 in)
{
    f32 s = 1.0f / 255.0f;

    return (Rgba){
        .r = RGBA32_R(in) * s,
        .g = RGBA32_G(in) * s,
        .b = RGBA32_B(in) * s,
        .a = RGBA32_A(in) * s,
    };
}

static inline Rgba32
rgbaPack32(Rgba in)
{
    return ((Rgba32)cfClamp(in.r * 255.0f, 0.0f, 255.0f)) << RGBA32_R_SHIFT |
           ((Rgba32)cfClamp(in.g * 255.0f, 0.0f, 255.0f)) << RGBA32_G_SHIFT |
           ((Rgba32)cfClamp(in.b * 255.0f, 0.0f, 255.0f)) << RGBA32_B_SHIFT |
           ((Rgba32)cfClamp(in.a * 255.0f, 0.0f, 255.0f)) << RGBA32_A_SHIFT;
}

static inline Rgba
rgbaMultiplyAlpha(Rgba col)
{
    if (col.a >= 1.0f) return col;

    return (Rgba){
        .r = col.r * col.a,
        .g = col.g * col.a,
        .b = col.b * col.a,
        .a = col.a,
    };
}

static inline Rgba
rgbaMultiplyAlpha32(Rgba32 col)
{
    Rgba rgba = rgbaUnpack32(col);

    CF_ASSERT(0.0f <= rgba.a && rgba.a <= 1.0f, "Alpha channel out of bounds");

    rgba.r *= rgba.a;
    rgba.g *= rgba.a;
    rgba.b *= rgba.a;

    return rgba;
}

// Convert rgba floats to hsva floats  (components in the [0-1] range), from Foley & van Dam p592
// Optimized http://lolengine.net/blog/2013/01/13/fast-rgb-to-hsv
static inline Hsva
rgbaToHsva(Rgba in)
{
    f32 K = 0.f;

    if (in.g < in.b)
    {
        cfSwapItem(in.g, in.b);
        K = -1.f;
    }

    if (in.r < in.g)
    {
        cfSwapItem(in.r, in.g);
        K = -2.f / 6.f - K;
    }

    f32 const chroma = in.r - (in.g < in.b ? in.g : in.b);

    // NOTE (Matteo): F32_MIN is added below to avoid checking against divisions by 0
    return (Hsva){
        .h = cfAbs(K + (in.g - in.b) / (6.f * chroma + F32_MIN)),
        .s = chroma / (in.r + F32_MIN),
        .v = in.r,
        .a = in.a,
    };
}

// Convert hsv floats to rgb floats (components in the [0-1] range), from Foley & van Dam p593 also
// http://en.wikipedia.org/wiki/HSL_and_HSV
static inline Rgba
hsvaToRgba(Hsva in)
{
    Rgba out = {.a = in.a};

    if (in.s == 0.0f)
    {
        // gray
        out.r = out.g = out.b = in.v;
    }
    else
    {
        f32 h = cfFmod(in.h, 1.0f) / (60.0f / 360.0f);
        i32 i = (i32)h;
        f32 f = h - (f32)i;
        f32 p = in.v * (1.0f - in.s);
        f32 q = in.v * (1.0f - in.s * f);
        f32 t = in.v * (1.0f - in.s * (1.0f - f));

        switch (i)
        {
            case 0:
                out.r = in.v;
                out.g = t;
                out.b = p;
                break;
            case 1:
                out.r = q;
                out.g = in.v;
                out.b = p;
                break;
            case 2:
                out.r = p;
                out.g = in.v;
                out.b = t;
                break;
            case 3:
                out.r = p;
                out.g = q;
                out.b = in.v;
                break;
            case 4:
                out.r = t;
                out.g = p;
                out.b = in.v;
                break;
            case 5:
            default:
                out.r = in.v;
                out.g = p;
                out.b = q;
                break;
        }
    }

    return out;
}

#define FOUNDATION_COLOR_H
#endif
