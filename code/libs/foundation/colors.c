#include "colors.h"
#include "error.h"
#include "math.inl"
#include "util.h"

float
colorSrgbEncode(float channel)
{
    if (channel > 0.0031308)
    {
        return cfMin(1.0f, 1.055f * mPow(channel, 1.0f / 2.4f) - 0.055f);
    }

    return cfMax(0.0f, 12.92f * channel);
}

float
colorSrgbDecode(float channel)
{
    if (channel > 0.04045)
    {
        return cfMin(1.0f, mPow((channel + 0.055f) / 1.055f, 2.4f));
    }

    return cfMax(0.0f, channel / 12.92f);
}

Srgb32
colorToSrgb(LinearColor in)
{
    return SRGB32(colorSrgbEncode(in.r) * 255.0f, //
                  colorSrgbEncode(in.g) * 255.0f, //
                  colorSrgbEncode(in.b) * 255.0f, //
                  cfClamp(in.a * 255.0f, 0.0f, 255.0f));
}

LinearColor
colorToLinear(Srgb32 in)
{
    float s = 1.0f / 255.0f;

    return (LinearColor){
        .r = colorSrgbDecode(SRGB32_R(in) * s),
        .g = colorSrgbDecode(SRGB32_G(in) * s),
        .b = colorSrgbDecode(SRGB32_B(in) * s),
        .a = SRGB32_A(in) * s,
    };
}

LinearColor
colorToLinearMultiplied(Srgb32 col)
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
_RgbToHsv(float rgb[3], float hsv[3])
{
    float K = 0.f;

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

    float const chroma = rgb[0] - (rgb[1] < rgb[2] ? rgb[1] : rgb[2]);

    // NOTE (Matteo): FLT_MIN is added below to avoid checking against divisions by 0
    hsv[0] = mAbs(K + (rgb[1] - rgb[2]) / (6.f * chroma + FLT_MIN));
    hsv[1] = chroma / (rgb[0] + FLT_MIN);
    hsv[2] = rgb[0];
}

// Convert hsv floats to rgb floats (components in the [0-1] range), from Foley & van Dam p593 also
// http://en.wikipedia.org/wiki/HSL_and_HSV
static void
_HsvToRgb(float hsv[3], float rgb[3])
{
    if (hsv[1] == 0.0f)
    {
        // gray
        rgb[0] = rgb[1] = rgb[2] = hsv[2];
    }
    else
    {
        float h = mFmod(hsv[0], 1.0f) * 6.0f;
        I32 sector_index = (I32)h;
        float f = h - (float)sector_index;
        float p = hsv[2] * (1.0f - hsv[1]);
        float q = hsv[2] * (1.0f - hsv[1] * f);
        float t = hsv[2] * (1.0f - hsv[1] * (1.0f - f));

        switch (sector_index)
        {
            case 0:
                rgb[0] = hsv[2];
                rgb[1] = t;
                rgb[2] = p;
                break;
            case 1:
                rgb[0] = q;
                rgb[1] = hsv[2];
                rgb[2] = p;
                break;
            case 2:
                rgb[0] = p;
                rgb[1] = hsv[2];
                rgb[2] = t;
                break;
            case 3:
                rgb[0] = p;
                rgb[1] = q;
                rgb[2] = hsv[2];
                break;
            case 4:
                rgb[0] = t;
                rgb[1] = p;
                rgb[2] = hsv[2];
                break;
            case 5:
            default:
                rgb[0] = hsv[2];
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
colorSrgbToHsv(Srgb32 in)
{
    float const ratio = 1.0f / 255.0f;
    float rgb[3] = {
        SRGB32_R(in) * ratio,
        SRGB32_G(in) * ratio,
        SRGB32_B(in) * ratio,
    };
    HsvColor out = {.a = SRGB32_A(in) * ratio};

    _RgbToHsv(rgb, out.elem);

    return out;
}

CF_API Srgb32
colorHsvToSrgb(HsvColor in)
{
    float rgb[3] = {0};

    _HsvToRgb(in.elem, rgb);

    return ((Srgb32)cfClamp(rgb[0] * 255.0f, 0.0f, 255.0f)) << SRGB32_R_SHIFT |
           ((Srgb32)cfClamp(rgb[1] * 255.0f, 0.0f, 255.0f)) << SRGB32_G_SHIFT |
           ((Srgb32)cfClamp(rgb[2] * 255.0f, 0.0f, 255.0f)) << SRGB32_B_SHIFT |
           ((Srgb32)cfClamp(in.a * 255.0f, 0.0f, 255.0f)) << SRGB32_A_SHIFT;
}
