#pragma once

//------------------------------------------------------------------------------
// Safely include cimgui.h with C declarations
//------------------------------------------------------------------------------

#include "foundation/common.h"

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

#if CF_COMP_MSVC
#    pragma warning(pop)
#elif CF_COMP_CLANG
#    pragma clang diagnostic pop
#endif

//------------------------------------------------------------------------------
// Gui utilities
//------------------------------------------------------------------------------

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

typedef struct Gui
{
    ImGuiContext *ctx;
    void *alloc_state;
    void *(*alloc_func)(Usize size, void *state);
    void (*free_func)(void *mem, void *state);
} Gui;

void guiInit(Gui *gui);

void guiBeginFullScreen(char *label, bool docking, bool menu_bar);
void guiEndFullScreen(void);

bool guiFontOptions(FontOptions *state);

bool guiCenteredButton(char const *label);

//------------------------------------------------------------------------------
// Inline utilities
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
