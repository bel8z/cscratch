#ifndef GUI_H

//------------------------------------------------------------------------------
// Safely include cimgui.h with C declarations
//------------------------------------------------------------------------------

#include "foundation/common.h"

#if CF_COMPILER_MSVC
#pragma warning(push)
#pragma warning(disable : 4201)
#pragma warning(disable : 4214)
#elif CF_COMPILER_CLANG
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wlanguage-extension-token"
#endif

#define CIMGUI_DEFINE_ENUMS_AND_STRUCTS
#include "cimgui.h"

#if CF_COMP_MSVC
#pragma warning(pop)
#elif CF_COMP_CLANG
#pragma clang diagnostic pop
#endif

#include "foundation/vec.h"

// Helper struct to tweak IMGUI font handling
typedef struct FontOptions
{
    I32 tex_glyph_padding;
    F32 rasterizer_multiply;
    // Stb only
    I32 oversample_h;
    I32 oversample_v;
    // Freetype only
    U32 freetype_flags;
    bool freetype_enabled;
} FontOptions;

//------------------------------------------------------------------------------
// Some common gui extensions
//------------------------------------------------------------------------------

void guiBeginFullScreen(char *label, bool docking, bool menu_bar);
void guiEndFullScreen(void);

bool guiFontOptions(FontOptions *state);

static inline bool
guiButton(char const *label)
{
    return igButton(label, (ImVec2){0, 0});
}

static inline bool
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

static inline void
guiSameLine()
{
    igSameLine(0.0f, -1.0f);
}

static inline bool
guiKeyPressed(ImGuiKey key)
{
    return igIsKeyPressed(igGetIO()->KeyMap[key], true);
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

//------------------------------------------------------------------------------

#define GUI_H
#endif
