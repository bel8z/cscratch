#pragma once

#include "foundation/core.h"

#if !defined(GUI_FREETYPE)
#    define GUI_FREETYPE 1
#endif

// NOTE (Matteo): Here i forward declare some Imgui internal types, while
// also "importing" them into the Gui namespace

typedef struct ImGuiContext GuiContext;
typedef struct ImFontAtlas GuiFontAtlas;
typedef struct ImDrawList GuiDrawList;
typedef struct ImDrawData GuiDrawData;
typedef struct ImFont GuiFont;
typedef struct ImGuiDockNode GuiDockNode;

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
    GuiKey_None = 0,

    GuiKey_Tab = 512,
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
    GuiKey_Apostrophe,   // '
    GuiKey_Comma,        // ,
    GuiKey_Minus,        // -
    GuiKey_Period,       // .
    GuiKey_Slash,        // /
    GuiKey_Semicolon,    // ;
    GuiKey_Equal,        // =
    GuiKey_LeftBracket,  // [
    GuiKey_Backslash,    // \ (this text inhibit multiline comment caused by backlash)
    GuiKey_RightBracket, // ]
    GuiKey_GraveAccent,  // `
    GuiKey_CapsLock,
    GuiKey_ScrollLock,
    GuiKey_NumLock,
    GuiKey_PrintScreen,
    GuiKey_Pause,
    GuiKey_Keypad0,
    GuiKey_Keypad1,
    GuiKey_Keypad2,
    GuiKey_Keypad3,
    GuiKey_Keypad4,
    GuiKey_Keypad5,
    GuiKey_Keypad6,
    GuiKey_Keypad7,
    GuiKey_Keypad8,
    GuiKey_Keypad9,
    GuiKey_KeypadDecimal,
    GuiKey_KeypadDivide,
    GuiKey_KeypadMultiply,
    GuiKey_KeypadSubtract,
    GuiKey_KeypadAdd,
    GuiKey_KeypadEnter,
    GuiKey_KeypadEqual,
    GuiKey_LeftCtrl,
    GuiKey_LeftShift,
    GuiKey_LeftAlt,
    GuiKey_LeftSuper,
    GuiKey_RightCtrl,
    GuiKey_RightShift,
    GuiKey_RightAlt,
    GuiKey_RightSuper,
    GuiKey_Menu,
    GuiKey_0,
    GuiKey_1,
    GuiKey_2,
    GuiKey_3,
    GuiKey_4,
    GuiKey_5,
    GuiKey_6,
    GuiKey_7,
    GuiKey_8,
    GuiKey_9,
    GuiKey_A,
    GuiKey_B,
    GuiKey_C,
    GuiKey_D,
    GuiKey_E,
    GuiKey_F,
    GuiKey_G,
    GuiKey_H,
    GuiKey_I,
    GuiKey_J,
    GuiKey_K,
    GuiKey_L,
    GuiKey_M,
    GuiKey_N,
    GuiKey_O,
    GuiKey_P,
    GuiKey_Q,
    GuiKey_R,
    GuiKey_S,
    GuiKey_T,
    GuiKey_U,
    GuiKey_V,
    GuiKey_W,
    GuiKey_X,
    GuiKey_Y,
    GuiKey_Z,
    GuiKey_F1,
    GuiKey_F2,
    GuiKey_F3,
    GuiKey_F4,
    GuiKey_F5,
    GuiKey_F6,
    GuiKey_F7,
    GuiKey_F8,
    GuiKey_F9,
    GuiKey_F10,
    GuiKey_F11,
    GuiKey_F12,

    GuiKey_COUNT, // No valid GuiKey is ever greater than this value
};

typedef I32 GuiMouseButton;
enum GuiMouseButton_
{
    GuiMouseButton_Left = 0,
    GuiMouseButton_Right = 1,
    GuiMouseButton_Middle = 2,
    GuiMouseButton_COUNT = 5
};

// Constants for DPI handling
#define GUI_PLATFORM_DPI 96.0f
#define GUI_TRUETYPE_DPI 72.0f

// Memory information
typedef struct GuiMemory
{
    Usize size;
    Usize blocks;
} GuiMemory;

CF_API void guiSetContext(GuiContext *ctx);

//=== Updating & querying ===//

CF_API GuiMemory guiMemoryInfo(void);
CF_API void *guiUserData(void);

CF_API void guiNewFrame();
CF_API GuiDrawData *guiRender();

//=== Themes & styling ===//

/// Custom IMGUI color themes
typedef Usize GuiTheme;

#define GUI_THEMES(X) \
    X(EmeraldDark)    \
    X(EmeraldLight)   \
    X(LightBlue)      \
    X(LightGreen)     \
    X(Rugged)         \
    X(ImguiDark)      \
    X(ImguiLight)     \
    X(ImguiClassic)   \
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

CF_API Srgb32 guiGetStyledColor(Srgb32 in);

CF_API Srgb32 guiGetBackColor(void);

CF_API void guiGammaCorrection(bool enabled);

//=== IO ===//

CF_API F32 guiGetFramerate(void);

CF_API bool guiKeyPressed(GuiKey key);
CF_API I32 guiKeyPressedCount(GuiKey key);
CF_API bool guiKeyCtrl(void);
CF_API bool guiKeyAlt(void);
CF_API bool guiKeyShift(void);

CF_API Vec2 guiGetMousePos(void);
CF_API Vec2 guiGetMouseDelta(void);
CF_API F32 guiGetMouseWheel(void);
CF_API bool guiGetMouseDragging(GuiMouseButton button, Vec2 *out_delta);
CF_API F32 guiGetMouseDownDuration(GuiMouseButton button);

//=== Windows ===//

typedef U8 GuiDockStyle;
enum GuiDockStyle_
{
    GuiDockStyle_Default = 0,
    GuiDockStyle_Transparent = 8,
    GuiDockStyle_CentralViewport = 4,
};

typedef struct GuiDockLayout
{
    GuiDockNode *node;
    U32 id;
    bool open;
} GuiDockLayout;

CF_API void guiDockSpace(GuiDockStyle style);

CF_API GuiDockLayout guiDockLayout(void);
CF_API U32 guiDockSplitUp(GuiDockLayout *layout, F32 size_ratio);
CF_API U32 guiDockSplitDown(GuiDockLayout *layout, F32 size_ratio);
CF_API U32 guiDockSplitLeft(GuiDockLayout *layout, F32 size_ratio);
CF_API U32 guiDockSplitRight(GuiDockLayout *layout, F32 size_ratio);
CF_API bool guiDockWindow(GuiDockLayout *layout, Cstr name, U32 dock_id);

CF_API void guiSetNextWindowSize(Vec2 size, GuiCond cond);

CF_API bool guiBegin(Cstr name, bool *p_open);
CF_API bool guiBeginAutoResize(Cstr name, bool *p_open);
CF_API bool guiBeginLayout(Cstr name, GuiDockLayout *layout);
CF_API void guiEnd(void);

CF_API void guiMetricsWindow(bool *p_open);
CF_API void guiDemoWindow(bool *p_open);
CF_API void guiPlotDemoWindow(bool *p_open);

// NOTE (Matteo): Deprecated?
CF_API void guiBeginFullScreen(Cstr label, bool docking, bool menu_bar);
CF_API void guiEndFullScreen(void);

//=== Modals ===//

CF_API bool guiBeginPopupModal(Cstr name, bool *p_open);
CF_API void guiEndPopup(void);

CF_API void guiOpenPopup(Cstr name);
CF_API void guiClosePopup(void);

//=== Widgets ===//

typedef struct GuiInputInfo
{
    F32 step;
    F32 step_fast;
    Cstr format;
} GuiInputInfo;

CF_API bool guiIsItemHovered(void);

CF_API bool guiButton(Cstr label);
CF_API bool guiCenteredButton(Cstr label);

CF_API bool guiCheckbox(Cstr label, bool *checked);

CF_API bool guiSlider(Cstr label, F32 *value, F32 min_value, F32 max_value);

CF_API bool guiInput(Cstr label, F32 *value, GuiInputInfo *info);

CF_API void guiText(Cstr fmt, ...) CF_PRINTF_LIKE(0);
CF_API void guiTextV(Cstr fmt, va_list args) CF_VPRINTF_LIKE(0);

CF_API bool guiBeginMainMenuBar(void);
CF_API void guiEndMainMenuBar(void);
CF_API bool guiBeginMenu(Cstr label, bool enabled);
CF_API void guiEndMenu(void);
CF_API bool guiMenuItem(Cstr label, bool *p_selected);

CF_API bool guiCombo(Cstr label, Cstr preview, Cstr const *values, Usize count,
                     Usize *selected_index);

/// Custom color edit with an additional combobox for choosing X11 named colors
CF_API bool guiColorEdit(Cstr label, Srgb32 *color);

CF_API void guiStyleEditor(void);

//=== Plots ===//

CF_API void guiTestPlot(Cstr label);

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
#if GUI_FREETYPE
    U32 freetype_flags;
    bool freetype_disabled;
#endif
} GuiFontOptions;

/// Widget for the editing of font options
CF_API bool guiFontOptionsEdit(GuiFontOptions *state);
/// Update the given atlas with the given options
CF_API void guiUpdateAtlas(GuiFontAtlas *fonts, GuiFontOptions *font_opts);
/// Update the current font atlas with the given options
#define guiUpdateFonts(font_opts) guiUpdateAtlas(igGetIO()->Fonts, font_opts)

CF_API GuiFontAtlas *guiFonts(void);
CF_API GuiFont *guiLoadFont(GuiFontAtlas *fonts, Cstr file_name, F32 font_size);
CF_API GuiFont *guiLoadDefaultFont(GuiFontAtlas *fonts);
CF_API bool guiLoadCustomFonts(GuiFontAtlas *fonts, F32 scale, Str data_path);

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
    ErrorCode32 error;
} GuiFileDialogResult;

CF_API GuiFileDialogResult guiFileDialog(GuiFileDialogParms *parms, MemAllocator alloc);

//=== Log ===//

typedef struct CfLog CfLog;

/// Widget for displaying log content
CF_API void guiLogBox(CfLog *log, bool readonly);

//=== Canvas ===//

typedef struct GuiCanvas
{
    Vec2 size, p0, p1;
    GuiDrawList *draw_list;
    Srgb32 stroke_color, fill_color;
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

CF_API void guiCanvasDrawText(GuiCanvas *canvas, Str text, Vec2 pos, Srgb32 color);

CF_API void guiCanvasDrawImage(GuiCanvas *canvas, U32 texture, //
                               Vec2 image_min, Vec2 image_max, //
                               Vec2 uv_min, Vec2 uv_max);

//=== Miscellanea ===//

CF_API void guiSameLine(void);
CF_API void guiSeparator(void);

//------------------------------------------------------------------------------
