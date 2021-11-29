#pragma once

#include "foundation/core.h"

//------------------------------------------------------------------------------
// Application interface
//------------------------------------------------------------------------------

typedef struct Platform Platform;
typedef struct CommandLine CommandLine;
typedef struct AppState AppState;
typedef struct GuiFontOptions GuiFontOptions;

// TODO (Matteo): Maybe refactor this a bit

/// Data that can be exchanged between application and platform
typedef struct AppIo
{
    // Inputs
    GuiFontOptions *font_opts;

    // Outputs
    Cstr window_title;
    Color32 back_color;
    bool quit;
    bool rebuild_fonts;
    bool continuous_update;
    bool fullscreen;
    bool window_title_changed;
} AppIo;

// Application function signatures

#define APP_PROC(name) void name(AppState *app)
#define APP_CREATE_PROC(name) AppState *name(Platform *plat, CommandLine *cmd_line)
#define APP_UPDATE_PROC(name) void name(AppState *state, AppIo *io)

typedef APP_PROC((*AppProc));
typedef APP_CREATE_PROC((*AppCreateProc));
typedef APP_UPDATE_PROC((*AppUpdateProc));

// Application library exports

#if CF_OS_WIN32
#    define APP_API __declspec(dllexport)
#else
#    define APP_API
#endif

APP_API APP_CREATE_PROC(appCreate);
APP_API APP_PROC(appDestroy);
APP_API APP_PROC(appLoad);
APP_API APP_PROC(appUnload);
APP_API APP_UPDATE_PROC(appUpdate);
