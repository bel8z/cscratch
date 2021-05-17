#include "gui.h"

bool
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
