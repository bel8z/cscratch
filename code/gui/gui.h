#pragma once

#include "foundation/core.h"

// Forward declared types

typedef struct ImGuiContext ImGuiContext;
typedef struct ImFontAtlas ImFontAtlas;
typedef struct ImDrawList ImDrawList;
typedef struct ImDrawData ImDrawData;
typedef struct ImFont ImFont;
typedef struct ImFontAtlas ImFontAtlas;
typedef struct ImGuiDockNode ImGuiDockNode;

// Enums

typedef I32 GuiCond;
enum GuiCond_
{
    GuiCond_None = 0,
    GuiCond_Always = 1 << 0,
    GuiCond_Once = 1 << 1,
    GuiCond_FirstUseEver = 1 << 2,
    GuiCond_Appearing = 1 << 3
};

typedef I32 GuiKey;
enum GuiKey_
{
    GuiKey_Tab,
    GuiKey_LeftArrow,
    GuiKey_RightArrow,
    GuiKey_UpArrow,
    GuiKey_DownArrow,
    GuiKey_PageUp,
    GuiKey_PageDown,
    GuiKey_Home,
    GuiKey_End,
    GuiKey_Insert,
    GuiKey_Delete,
    GuiKey_Backspace,
    GuiKey_Space,
    GuiKey_Enter,
    GuiKey_Escape,
    GuiKey_KeyPadEnter,
    GuiKey_A,
    GuiKey_C,
    GuiKey_V,
    GuiKey_X,
    GuiKey_Y,
    GuiKey_Z,
    GuiKey_COUNT
};

typedef I32 GuiMouseButton;
enum GuiMouseButton_
{
    GuiMouseButton_Left = 0,
    GuiMouseButton_Right = 1,
    GuiMouseButton_Middle = 2,
    GuiMouseButton_COUNT = 5
};

//=== Initialization ===//

/// IMGUI state, used to initialize internal global variables
typedef struct Gui
{
    ImGuiContext *ctx;
    MemAllocator *alloc;
    ImFontAtlas *shared_atlas;
    Cstr ini_filename;
} Gui;

/// Initialize IMGUI global state
void guiInit(Gui *gui);
void guiShutdown(Gui *gui);

bool guiViewportsEnabled(void);

void guiNewFrame(void);
ImDrawData *guiRender(void);
void guiUpdateAndRenderViewports(void);

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

Rgba32 guiGetStyledColor(Rgba32 in);

Rgba32 guiGetBackColor(void);

//=== IO ===//

F32 guiGetFramerate(void);

bool guiKeyPressed(GuiKey key);
bool guiKeyCtrl(void);
bool guiKeyAlt(void);
bool guiKeyShift(void);

Vec2 guiGetMousePos(void);
F32 guiGetMouseWheel(void);
bool guiGetMouseDragging(GuiMouseButton button, Vec2 *out_delta);
F32 guiGetMouseDownDuration(GuiMouseButton button);

//=== Windows ===//

typedef struct GuiDockLayout
{
    ImGuiDockNode *node;
    U32 id;
    bool open;
} GuiDockLayout;

GuiDockLayout guiDockLayout(void);
U32 guiDockSplitUp(GuiDockLayout *layout, F32 size_ratio);
U32 guiDockSplitDown(GuiDockLayout *layout, F32 size_ratio);
U32 guiDockSplitLeft(GuiDockLayout *layout, F32 size_ratio);
U32 guiDockSplitRight(GuiDockLayout *layout, F32 size_ratio);
bool guiDockWindow(GuiDockLayout *layout, Cstr name, U32 dock_id);

void guiSetNextWindowSize(Vec2 size, GuiCond cond);

bool guiBegin(Cstr name, bool *p_open);
bool guiBeginLayout(Cstr name, GuiDockLayout *layout);
void guiEnd(void);

void guiMetricsWindow(bool *p_open);
void guiDemoWindow(bool *p_open);

// NOTE (Matteo): Deprecated?
void guiBeginFullScreen(Cstr label, bool docking, bool menu_bar);
void guiEndFullScreen(void);

//=== Modals ===//

bool guiBeginPopupModal(Cstr name, bool *p_open);
void guiEndPopup(void);

void guiOpenPopup(Cstr name);
void guiClosePopup(void);

//=== Widgets ===//

bool guiIsItemHovered(void);

bool guiButton(Cstr label);
bool guiCenteredButton(Cstr label);

bool guiCheckbox(Cstr label, bool *checked);

bool guiSlider(Cstr label, F32 *value, F32 min_value, F32 max_value);

CF_PRINTF_LIKE(0, 1)
void guiText(Cstr fmt, ...);

bool guiBeginMainMenuBar(void);
void guiEndMainMenuBar(void);
bool guiBeginMenu(Cstr label, bool enabled);
void guiEndMenu(void);
bool guiMenuItem(Cstr label, bool *p_selected);

/// Custom color edit with an additional combobox for choosing X11 named colors
bool guiColorEdit(Cstr label, Rgba32 *color);

void guiStyleEditor(void);

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

ImFontAtlas *guiFonts(void);
ImFont *guiLoadFont(ImFontAtlas *fonts, Cstr file_name, F32 font_size);
ImFont *guiLoadDefaultFont(ImFontAtlas *fonts);

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

void guiSameLine(void);
void guiSeparator(void);

//------------------------------------------------------------------------------
