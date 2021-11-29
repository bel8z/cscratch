#include "colors.h"
#include "math.inl"
#include "util.h"

Color32
colorToSrgb(LinearColor in)
{
    return ((Color32)cfClamp(mPow(in.r, 1.0f / 2.2f) * 255.0f, 0.0f, 255.0f)) << RGBA32_R_SHIFT |
           ((Color32)cfClamp(mPow(in.g, 1.0f / 2.2f) * 255.0f, 0.0f, 255.0f)) << RGBA32_G_SHIFT |
           ((Color32)cfClamp(mPow(in.b, 1.0f / 2.2f) * 255.0f, 0.0f, 255.0f)) << RGBA32_B_SHIFT |
           ((Color32)cfClamp(in.a * 255.0f, 0.0f, 255.0f)) << RGBA32_A_SHIFT;
}

LinearColor
colorToLinear(Color32 in)
{
    F32 s = 1.0f / 255.0f;

    return (LinearColor){
        .r = mPow(RGBA32_R(in) * s, 2.2f),
        .g = mPow(RGBA32_G(in) * s, 2.2f),
        .b = mPow(RGBA32_B(in) * s, 2.2f),
        .a = mPow(RGBA32_A(in) * s, 2.2f),
    };
}

LinearColor
colorToLinearMultiplied(Color32 col)
{
    LinearColor rgba = colorToLinear(col);

    CF_ASSERT(0.0f <= rgba.a && rgba.a <= 1.0f, "Alpha channel out of bounds");

    rgba.r *= rgba.a;
    rgba.g *= rgba.a;
    rgba.b *= rgba.a;

    return rgba;
}

LinearColor
colorMultiplyAlpha(LinearColor col)
{
    if (col.a >= 1.0f) return col;

    return (LinearColor){
        .r = col.r * col.a,
        .g = col.g * col.a,
        .b = col.b * col.a,
        .a = col.a,
    };
}

// Convert rgba floats to hsva floats  (components in the [0-1] range), from Foley & van Dam p592
// Optimized http://lolengine.net/blog/2013/01/13/fast-rgb-to-hsv
static void
_RgbToHsv(F32 rgb[3], F32 hsv[3])
{
    F32 K = 0.f;

    if (rgb[1] < rgb[2])
    {
        cfSwapItem(rgb[1], rgb[2]);
        K = -1.f;
    }

    if (rgb[0] < rgb[1])
    {
        cfSwapItem(rgb[0], rgb[1]);
        K = -2.f / 6.f - K;
    }

    F32 const chroma = rgb[0] - (rgb[1] < rgb[2] ? rgb[1] : rgb[2]);

    // NOTE (Matteo): F32_MIN is added below to avoid checking against divisions by 0
    hsv[0] = mAbs(K + (rgb[1] - rgb[2]) / (6.f * chroma + F32_MIN));
    hsv[1] = chroma / (rgb[0] + F32_MIN);
    hsv[2] = rgb[0];
}

// Convert hsv floats to rgb floats (components in the [0-1] range), from Foley & van Dam p593 also
// http://en.wikipedia.org/wiki/HSL_and_HSV
static void
_HsvToRgb(F32 hsv[3], F32 rgb[3])
{
    if (hsv[1] == 0.0f)
    {
        // gray
        rgb[0] = rgb[1] = rgb[2] = hsv[3];
    }
    else
    {
        F32 h = mFmod(hsv[0], 1.0f) * 6.0f;
        I32 sector_index = (I32)h;
        F32 f = h - (F32)sector_index;
        F32 p = hsv[2] * (1.0f - hsv[1]);
        F32 q = hsv[2] * (1.0f - hsv[1] * f);
        F32 t = hsv[2] * (1.0f - hsv[1] * (1.0f - f));

        switch (sector_index)
        {
            case 0:
                rgb[0] = hsv[3];
                rgb[1] = t;
                rgb[2] = p;
                break;
            case 1:
                rgb[0] = q;
                rgb[1] = hsv[3];
                rgb[2] = p;
                break;
            case 2:
                rgb[0] = p;
                rgb[1] = hsv[3];
                rgb[2] = t;
                break;
            case 3:
                rgb[0] = p;
                rgb[1] = q;
                rgb[2] = hsv[3];
                break;
            case 4:
                rgb[0] = t;
                rgb[1] = p;
                rgb[2] = hsv[3];
                break;
            case 5:
            default:
                rgb[0] = hsv[3];
                rgb[1] = p;
                rgb[2] = q;
                break;
        }
    }
}

HsvColor
colorLinearToHsv(LinearColor in)
{
    // NOTE (Matteo): Convert to sRGB space first
    in.r = mPow(in.r, 1.0f / 2.2f);
    in.g = mPow(in.g, 1.0f / 2.2f);
    in.b = mPow(in.b, 1.0f / 2.2f);

    HsvColor out = {.a = in.a};
    _RgbToHsv(in.channel, out.elem);

    return out;
}

LinearColor
colorHsvToLinear(HsvColor in)
{
    LinearColor out = {.a = in.a};

    _HsvToRgb(in.elem, out.channel);

    // NOTE (Matteo): Convert to linear space
    out.r = mPow(out.r, 2.2f);
    out.g = mPow(out.g, 2.2f);
    out.b = mPow(out.b, 2.2f);

    return out;
}

HsvColor
colorSrgbToHsv(Color32 in)
{
    F32 const ratio = 1.0f / 255.0f;
    F32 rgb[3] = {
        RGBA32_R(in) * ratio,
        RGBA32_G(in) * ratio,
        RGBA32_B(in) * ratio,
    };
    HsvColor out = {.a = RGBA32_A(in) * ratio};

    _RgbToHsv(rgb, out.elem);

    return out;
}

CF_API Color32
colorHsvToSrgb(HsvColor in)
{
    F32 rgb[3] = {0};

    _HsvToRgb(in.elem, rgb);

    return ((Color32)cfClamp(rgb[0] * 255.0f, 0.0f, 255.0f)) << RGBA32_R_SHIFT |
           ((Color32)cfClamp(rgb[1] * 255.0f, 0.0f, 255.0f)) << RGBA32_G_SHIFT |
           ((Color32)cfClamp(rgb[2] * 255.0f, 0.0f, 255.0f)) << RGBA32_B_SHIFT |
           ((Color32)cfClamp(in.a * 255.0f, 0.0f, 255.0f)) << RGBA32_A_SHIFT;
}
