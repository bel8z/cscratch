#pragma once

//------------------------------------------------------------------------------
// Safely include cimgui.h with C declarations
//------------------------------------------------------------------------------

#include "foundation/core.h"

#if CF_COMPILER_MSVC
#    pragma warning(push)
#    pragma warning(disable : 4201)
#    pragma warning(disable : 4214)
#elif CF_COMPILER_CLANG
#    pragma clang diagnostic push
#    pragma clang diagnostic ignored "-Wlanguage-extension-token"
#endif

#define CIMGUI_DEFINE_ENUMS_AND_STRUCTS
#include "cimgui.h"

#if CF_COMPILER_MSVC
#    pragma warning(pop)
#elif CF_COMPILER_CLANG
#    pragma clang diagnostic pop
#endif

//------------------------------------------------------------------------------
// Gui utilities
//------------------------------------------------------------------------------

/// IMGUI state, used to initialize internal global variables
typedef struct Gui
{
    ImGuiContext *ctx;
    CfAllocator *alloc;
} Gui;

/// Helper struct to tweak IMGUI font handling
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

/// Initialize IMGUI global state
void guiInit(Gui *gui);

void guiBeginFullScreen(Cstr label, bool docking, bool menu_bar);
void guiEndFullScreen(void);

/// Widget for the editing of font options
bool guiFontOptionsEdit(FontOptions *state);
/// Update the given atlas with the given options
void guiUpdateAtlas(ImFontAtlas *fonts, FontOptions *font_opts);
/// Update the current font atlas with the given options
#define guiUpdateFonts(font_opts) guiUpdateAtlas(igGetIO()->Fonts, font_opts)

bool guiCenteredButton(Cstr label);

/// Custom color edit with an additional combobox for choosing X11 named colors
bool guiColorEdit(Cstr label, Rgba32 *color);

//------------------------------------------------------------------------------
// Inline utilities
//------------------------------------------------------------------------------

static inline bool
guiButton(Cstr label)
{
    return igButton(label, (ImVec2){0});
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
