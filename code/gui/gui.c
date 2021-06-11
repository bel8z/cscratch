#include "gui.h"

void
guiInit(Gui *gui)
{
    igDebugCheckVersionAndDataLayout("1.82", sizeof(ImGuiIO), sizeof(ImGuiStyle), sizeof(ImVec2),
                                     sizeof(ImVec4), sizeof(ImDrawVert), sizeof(ImDrawIdx));
    igSetAllocatorFunctions(gui->alloc_func, gui->free_func, gui->alloc_state);

    if (gui->ctx)
    {
        igSetCurrentContext(gui->ctx);
    }
    else
    {
        gui->ctx = igCreateContext(NULL);
    }
}

void
guiBeginFullScreen(char *label, bool docking, bool menu_bar)
{
    ImGuiViewport const *viewport = igGetMainViewport();
    igSetNextWindowPos(viewport->WorkPos, 0, (ImVec2){0, 0});
    igSetNextWindowSize(viewport->WorkSize, 0);
    igPushStyleVarFloat(ImGuiStyleVar_WindowRounding, 0.0f);
    igPushStyleVarFloat(ImGuiStyleVar_WindowBorderSize, 0.0f);
    igPushStyleVarVec2(ImGuiStyleVar_WindowPadding, (ImVec2){0.0f, 0.0f});

    ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse |
                                    ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                                    ImGuiWindowFlags_NoBringToFrontOnFocus |
                                    ImGuiWindowFlags_NoNavFocus;

    if (!docking) window_flags |= ImGuiWindowFlags_NoDocking;
    if (menu_bar) window_flags |= ImGuiWindowFlags_MenuBar;

    igBegin(label, NULL, window_flags);
    igPopStyleVar(3);
}

void
guiEndFullScreen(void)
{
    igEnd();
}

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

bool
guiCenteredButton(char const *label)
{
    ImGuiStyle *style = igGetStyle();

    // NOTE (Matteo): Button size calculation copied from ImGui::ButtonEx
    ImVec2 label_size, button_size;
    igCalcTextSize(&label_size, label, NULL, false, -1.0f);
    igCalcItemSize(&button_size, (ImVec2){0, 0}, //
                   label_size.x + style->FramePadding.x * 2.0f,
                   label_size.y + style->FramePadding.y * 2.0f);

    ImVec2 available_size;
    igGetContentRegionAvail(&available_size);

    if (available_size.x > button_size.x)
    {
        igSetCursorPosX((available_size.x - button_size.x) / 2);
    }

    return guiButton(label);
}
