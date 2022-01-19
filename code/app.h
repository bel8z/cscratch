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
    Srgb32 back_color;
    bool quit;
    bool rebuild_fonts;
    bool continuous_update;
    bool fullscreen;
    bool window_title_changed;
} AppIo;

// Application function signatures

#define APP_FN(name) void name(AppState *app)
#define APP_CREATE_FN(name) AppState *name(Platform *plat, CommandLine *cmd_line)
#define APP_UPDATE_FN(name) void name(AppState *state, AppIo *io)

typedef APP_FN((*AppProc));
typedef APP_CREATE_FN((*AppCreateProc));
typedef APP_UPDATE_FN((*AppUpdateProc));

// Application library exports

#if CF_OS_WIN32
#    define APP_API __declspec(dllexport)
#else
#    define APP_API
#endif

APP_API APP_CREATE_FN(appCreate);
APP_API APP_FN(appDestroy);
APP_API APP_FN(appLoad);
APP_API APP_FN(appUnload);
APP_API APP_UPDATE_FN(appUpdate);
