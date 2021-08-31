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

//=== Initialization ===//

/// IMGUI state, used to initialize internal global variables
typedef struct Gui
{
    ImGuiContext *ctx;
    MemAllocator *alloc;
    ImFontAtlas *shared_atlas;
} Gui;

/// Initialize IMGUI global state
void guiInit(Gui *gui);

//=== Themes & styling ===//

/// Custom IMGUI color themes
typedef enum GuiTheme
{
    GuiTheme_Dark,
    GuiTheme_Light,

    GuiTheme_Count,
} GuiTheme;

/// Set a custom IMGUI color theme and sizes according to scale
void guiSetupStyle(GuiTheme theme, F32 dpi_scale);

/// Set a custom IMGUI color theme
void guiSetTheme(GuiTheme theme);

//=== Fonts handling ===//

/// Helper struct to tweak IMGUI font handling
typedef struct GuiFontOptions
{
    I32 tex_glyph_padding;
    F32 rasterizer_multiply;
    // Stb only
    I32 oversample_h;
    I32 oversample_v;
    // Freetype only
    U32 freetype_flags;
    bool freetype_enabled;
} GuiFontOptions;

/// Widget for the editing of font options
bool guiFontOptionsEdit(GuiFontOptions *state);
/// Update the given atlas with the given options
void guiUpdateAtlas(ImFontAtlas *fonts, GuiFontOptions *font_opts);
/// Update the current font atlas with the given options
#define guiUpdateFonts(font_opts) guiUpdateAtlas(igGetIO()->Fonts, font_opts)

//=== File dialogs ===//

enum
{
    GuiFileDialog_Open,
    GuiFileDialog_Save,
};

typedef struct GuiFileDialogFilter
{
    // Display name of the filter
    Cstr name;
    // Supported extensions
    Cstr *extensions;
    Usize num_extensions;
} GuiFileDialogFilter;

typedef struct GuiFileDialogParms
{
    Str filename_hint;

    GuiFileDialogFilter *filters;
    Usize num_filters;

    U8 type;
} GuiFileDialogParms;

enum
{
    GuiFileDialogResult_Ok,
    GuiFileDialogResult_Cancel,
    GuiFileDialogResult_Error,
};

typedef struct GuiFileDialogResult
{
    Str filename;
    U8 code;
} GuiFileDialogResult;

GuiFileDialogResult guiFileDialog(GuiFileDialogParms *parms, MemAllocator alloc);

//=== Log ===//

typedef struct CfLog CfLog;

/// Widget for displaying log content
void guiLogBox(CfLog *log, bool readonly);

//=== Miscellanea ===//

void guiBeginFullScreen(Cstr label, bool docking, bool menu_bar);
void guiEndFullScreen(void);

bool guiCenteredButton(Cstr label);

/// Custom color edit with an additional combobox for choosing X11 named colors
bool guiColorEdit(Cstr label, Rgba32 *color);

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

// clang-format off
#define guiCastV2(v)                                  \
    _Generic((v),                                     \
             ImVec2 : (Vec2)  { .x = v.x, .y = v.y }, \
             Vec2   : (ImVec2){ .x = v.x, .y = v.y })

#define guiCastV4(v)                                                      \
    _Generic((v),                                                         \
             ImVec4 : (Vec4)  { .x = v.x, .y = v.y, .z = v.z, .w = v.w }, \
             Vec4   : (ImVec4){ .x = v.x, .y = v.y, .z = v.z, .w = v.w })

#define guiCastRgba(c)                                \
    _Generic((c),                                     \
             ImVec4 : (Rgba)  { .r = c.x, .g = c.y, .b = c.z, .a = c.w }, \
             Vec2   : (ImVec2){ .x = c.r, .y = c.g, .z = c.b, .w = c.a })
// clang-format on

//------------------------------------------------------------------------------
