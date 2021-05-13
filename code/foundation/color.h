#ifndef FOUNDATION_COLOR_H

#include "common.h"
#include "util.h"

typedef u32 Rgba32;

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

typedef union Rgba
{
    struct
    {
        f32 r, g, b, a;
    };

    f32 channel[4];
} Rgba;

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
    return (Rgba){col.r * col.a, col.g * col.a, col.b * col.a, col.a};
}

#define FOUNDATION_COLOR_H
#endif
