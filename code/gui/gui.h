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
CF_API void guiInit(Gui *gui);
CF_API void guiShutdown(Gui *gui);

CF_API bool guiViewportsEnabled(void);

CF_API void guiNewFrame(void);
CF_API ImDrawData *guiRender(void);
CF_API void guiUpdateAndRenderViewports(void);

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
CF_API void guiSetupStyle(GuiTheme theme, F32 dpi_scale);

/// Set a custom IMGUI color theme
CF_API void guiSetTheme(GuiTheme theme);

CF_API void guiThemeSelector(Cstr label);

CF_API Rgba32 guiGetStyledColor(Rgba32 in);

CF_API Rgba32 guiGetBackColor(void);

//=== IO ===//

CF_API F32 guiGetFramerate(void);

CF_API bool guiKeyPressed(GuiKey key);
CF_API bool guiKeyCtrl(void);
CF_API bool guiKeyAlt(void);
CF_API bool guiKeyShift(void);

CF_API Vec2 guiGetMousePos(void);
CF_API F32 guiGetMouseWheel(void);
CF_API bool guiGetMouseDragging(GuiMouseButton button, Vec2 *out_delta);
CF_API F32 guiGetMouseDownDuration(GuiMouseButton button);

//=== Windows ===//

typedef struct GuiDockLayout
{
    ImGuiDockNode *node;
    U32 id;
    bool open;
} GuiDockLayout;

CF_API GuiDockLayout guiDockLayout(void);
CF_API U32 guiDockSplitUp(GuiDockLayout *layout, F32 size_ratio);
CF_API U32 guiDockSplitDown(GuiDockLayout *layout, F32 size_ratio);
CF_API U32 guiDockSplitLeft(GuiDockLayout *layout, F32 size_ratio);
CF_API U32 guiDockSplitRight(GuiDockLayout *layout, F32 size_ratio);
CF_API bool guiDockWindow(GuiDockLayout *layout, Cstr name, U32 dock_id);

CF_API void guiSetNextWindowSize(Vec2 size, GuiCond cond);

CF_API bool guiBegin(Cstr name, bool *p_open);
CF_API bool guiBeginLayout(Cstr name, GuiDockLayout *layout);
CF_API void guiEnd(void);

CF_API void guiMetricsWindow(bool *p_open);
CF_API void guiDemoWindow(bool *p_open);

// NOTE (Matteo): Deprecated?
CF_API void guiBeginFullScreen(Cstr label, bool docking, bool menu_bar);
CF_API void guiEndFullScreen(void);

//=== Modals ===//

CF_API bool guiBeginPopupModal(Cstr name, bool *p_open);
CF_API void guiEndPopup(void);

CF_API void guiOpenPopup(Cstr name);
CF_API void guiClosePopup(void);

//=== Widgets ===//

CF_API bool guiIsItemHovered(void);

CF_API bool guiButton(Cstr label);
CF_API bool guiCenteredButton(Cstr label);

CF_API bool guiCheckbox(Cstr label, bool *checked);

CF_API bool guiSlider(Cstr label, F32 *value, F32 min_value, F32 max_value);

CF_API void guiText(Cstr fmt, ...) CF_PRINTF_LIKE(0, 1);

CF_API bool guiBeginMainMenuBar(void);
CF_API void guiEndMainMenuBar(void);
CF_API bool guiBeginMenu(Cstr label, bool enabled);
CF_API void guiEndMenu(void);
CF_API bool guiMenuItem(Cstr label, bool *p_selected);

/// Custom color edit with an additional combobox for choosing X11 named colors
CF_API bool guiColorEdit(Cstr label, Rgba32 *color);

CF_API void guiStyleEditor(void);

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
CF_API bool guiFontOptionsEdit(GuiFontOptions *state);
/// Update the given atlas with the given options
CF_API void guiUpdateAtlas(ImFontAtlas *fonts, GuiFontOptions *font_opts);
/// Update the current font atlas with the given options
#define guiUpdateFonts(font_opts) guiUpdateAtlas(igGetIO()->Fonts, font_opts)

CF_API ImFontAtlas *guiFonts(void);
CF_API ImFont *guiLoadFont(ImFontAtlas *fonts, Cstr file_name, F32 font_size);
CF_API ImFont *guiLoadDefaultFont(ImFontAtlas *fonts);

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

CF_API GuiFileDialogResult guiFileDialog(GuiFileDialogParms *parms, MemAllocator alloc);

//=== Log ===//

typedef struct CfLog CfLog;

/// Widget for displaying log content
CF_API void guiLogBox(CfLog *log, bool readonly);

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
} GuiCanvas;

CF_API void guiCanvasBegin(GuiCanvas *canvas);
CF_API void guiCanvasEnd(GuiCanvas *canvas);

CF_API void guiCanvasDrawLine(GuiCanvas *canvas, Vec2 p0, Vec2 p1);
CF_API void guiCanvasDrawPolyline(GuiCanvas *canvas, Vec2 points[], Usize count);

CF_API void guiCanvasDrawRect(GuiCanvas *canvas, Vec2 p0, Vec2 p1);
CF_API void guiCanvasFillRect(GuiCanvas *canvas, Vec2 p0, Vec2 p1);

CF_API void guiCanvasDrawCircle(GuiCanvas *canvas, Vec2 center, F32 radius);
CF_API void guiCanvasFillCircle(GuiCanvas *canvas, Vec2 center, F32 radius);

CF_API void guiCanvasDrawText(GuiCanvas *canvas, Str text, Vec2 pos, Rgba32 color);

CF_API void guiCanvasDrawImage(GuiCanvas *canvas, U32 texture, //
                               Vec2 image_min, Vec2 image_max, //
                               Vec2 uv_min, Vec2 uv_max);

CF_API void guiCanvasPushStrokeColor(GuiCanvas *canvas, Rgba32 color);
CF_API void guiCanvasPopStrokeColor(GuiCanvas *canvas);

//=== Miscellanea ===//

CF_API void guiSameLine(void);
CF_API void guiSeparator(void);

//------------------------------------------------------------------------------
