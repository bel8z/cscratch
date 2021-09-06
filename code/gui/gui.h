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
typedef Usize GuiTheme;

#define GUI_THEMES(X) \
    X(Dark)           \
    X(Light)          \
    X(Dummy)

enum GuiTheme_
{
#define GUI_THEME_ENUM(Name) GuiTheme_##Name,
    GUI_THEMES(GUI_THEME_ENUM) GuiTheme_Count,
#undef GUI_THEME_ENUM
};

/// Set a custom IMGUI color theme and sizes according to scale
void guiSetupStyle(GuiTheme theme, F32 dpi_scale);

/// Set a custom IMGUI color theme
void guiSetTheme(GuiTheme theme);

void guiThemeSelector(Cstr label);

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

//=== Canvas ===//

enum
{
    GuiCanvas_StackSize = 16
};

typedef struct GuiCanvas
{
    Vec2 size, p0, p1;
    ImDrawList *draw_list;
    Rgba32 stroke_color, fill_color;
    F32 stroke_thick;

    CfBuffer(Rgba32, GuiCanvas_StackSize) stroke_color_stack;
    CfBuffer(Rgba32, GuiCanvas_StackSize) stroke_thick_stack;
    CfBuffer(Rgba32, GuiCanvas_StackSize) fill_color_stack;

} GuiCanvas;

void guiCanvasBegin(GuiCanvas *canvas);
void guiCanvasEnd(GuiCanvas *canvas);

void guiCanvasDrawLine(GuiCanvas *canvas, Vec2 p0, Vec2 p1);
void guiCanvasDrawPolyline(GuiCanvas *canvas, Vec2 points[], Usize count);

void guiCanvasDrawRect(GuiCanvas *canvas, Vec2 p0, Vec2 p1);
void guiCanvasFillRect(GuiCanvas *canvas, Vec2 p0, Vec2 p1);

void guiCanvasDrawCircle(GuiCanvas *canvas, Vec2 center, F32 radius);
void guiCanvasFillCircle(GuiCanvas *canvas, Vec2 center, F32 radius);

void guiCanvasDrawText(GuiCanvas *canvas, Str text, Vec2 pos, Rgba32 color);

void guiCanvasDrawImage(GuiCanvas *canvas, U32 texture, //
                        Vec2 image_min, Vec2 image_max, //
                        Vec2 uv_min, Vec2 uv_max);

void guiCanvasPushStrokeColor(GuiCanvas *canvas, Rgba32 color);
void guiCanvasPopStrokeColor(GuiCanvas *canvas);

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

CF_STATIC_ASSERT(sizeof(Vec2) == sizeof(ImVec2), "Vec2 not compatible with ImVec2");
CF_STATIC_ASSERT(sizeof(Vec4) == sizeof(ImVec4), "Vec4 not compatible with ImVec4");
CF_STATIC_ASSERT(sizeof(Rgba) == sizeof(ImVec4), "Rgba not compatible with ImVec4");

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
