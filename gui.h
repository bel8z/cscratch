#ifndef GUI_H

//------------------------------------------------------------------------------
// Safely include cimgui.h with C declarations
//------------------------------------------------------------------------------

#if defined(_MSC_VER)
#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wlanguage-extension-token"
#else
#pragma warning(push)
#pragma warning(disable : 4201)
#pragma warning(disable : 4214)
#endif // defined(__clang__)
#endif // defined(_MSC_VER)

#define CIMGUI_DEFINE_ENUMS_AND_STRUCTS
#include "cimgui.h"

#if defined(_MSC_VER)
#if defined(__clang__)
#pragma clang diagnostic pop
#else
#pragma warning(pop)
#endif // defined(__clang__)
#endif // defined(_MSC_VER)

#include "foundation/vec.h"

// Helper struct to tweak IMGUI font handling
typedef struct FontOptions
{
    i32 tex_glyph_padding;
    f32 rasterizer_multiply;
    // Freetype only
    bool freetype_enabled;
    u32 freetype_flags;
    // Stb only
    i32 oversample_h;
    i32 oversample_v;
} FontOptions;

//------------------------------------------------------------------------------
// Some common gui extensions
//------------------------------------------------------------------------------

static inline bool
guiButton(char const *label)
{
    return igButton(label, (ImVec2){0, 0});
}

static inline void
guiSameLine()
{
    igSameLine(0.0f, -1.0f);
}

static inline ImVec2
guiFromV2(Vec2 v)
{
    return (ImVec2){.x = v.x, .y = v.y};
}

static inline Vec2
guiToV2(ImVec2 v)
{
    return (Vec2){.x = v.x, .y = v.y};
}

static inline ImVec4
guiFromV4(Vec4 v)
{
    return (ImVec4){.x = v.x, .y = v.y, .z = v.z, .w = v.w};
}

static inline Vec4
guiToV4(ImVec4 v)
{
    return (Vec4){.x = v.x, .y = v.y, .z = v.z, .w = v.w};
}

static bool
guiFontOptions(FontOptions *state)
{
    bool rebuild_fonts = false;

    igShowFontSelector("Fonts");

    if (igRadioButtonBool("FreeType", state->freetype_enabled))
    {
        state->freetype_enabled = true;
        rebuild_fonts = true;
    }
    guiSameLine();
    if (igRadioButtonBool("Stb (Default)", !state->freetype_enabled))
    {
        state->freetype_enabled = false;
        rebuild_fonts = true;
    }

    rebuild_fonts |= igDragInt("TexGlyphPadding", &state->tex_glyph_padding, 0.1f, 1, 16, NULL,
                               ImGuiSliderFlags_None);

    rebuild_fonts |= igDragFloat("RasterizerMultiply", &state->rasterizer_multiply, 0.001f, 0.0f,
                                 2.0f, NULL, ImGuiSliderFlags_None);

    igSeparator();

    if (state->freetype_enabled)
    {

        rebuild_fonts |= igCheckboxFlagsUintPtr("NoHinting", &state->freetype_flags,
                                                ImGuiFreeTypeBuilderFlags_NoHinting);
        rebuild_fonts |= igCheckboxFlagsUintPtr("NoAutoHint", &state->freetype_flags,
                                                ImGuiFreeTypeBuilderFlags_NoAutoHint);
        rebuild_fonts |= igCheckboxFlagsUintPtr("ForceAutoHint", &state->freetype_flags,
                                                ImGuiFreeTypeBuilderFlags_ForceAutoHint);
        rebuild_fonts |= igCheckboxFlagsUintPtr("LightHinting", &state->freetype_flags,
                                                ImGuiFreeTypeBuilderFlags_LightHinting);
        rebuild_fonts |= igCheckboxFlagsUintPtr("MonoHinting", &state->freetype_flags,
                                                ImGuiFreeTypeBuilderFlags_MonoHinting);
        rebuild_fonts |=
            igCheckboxFlagsUintPtr("Bold", &state->freetype_flags, ImGuiFreeTypeBuilderFlags_Bold);
        rebuild_fonts |= igCheckboxFlagsUintPtr("Oblique", &state->freetype_flags,
                                                ImGuiFreeTypeBuilderFlags_Oblique);
        rebuild_fonts |= igCheckboxFlagsUintPtr("Monochrome", &state->freetype_flags,
                                                ImGuiFreeTypeBuilderFlags_Monochrome);
    }
    else
    {
        rebuild_fonts |= igDragInt("Oversample H", &state->oversample_h, 0.1f, 1, 5, NULL,
                                   ImGuiSliderFlags_None);
        rebuild_fonts |= igDragInt("Oversample V", &state->oversample_v, 0.1f, 1, 5, NULL,
                                   ImGuiSliderFlags_None);
    }

    return rebuild_fonts;
}

//------------------------------------------------------------------------------

#define GUI_H
#endif
