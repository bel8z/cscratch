#pragma once

#include "foundation/core.h"

//------------------------------------------------------------------------------
// Platform interface
//------------------------------------------------------------------------------

// Foundation interfaces
typedef struct CfVirtualMemory CfVirtualMemory;
typedef struct CfAllocator CfAllocator;
typedef struct CfFileSystem CfFileSystem;

// Additional platform interfaces
typedef struct GlApi GlApi;
typedef struct Gui Gui;
typedef struct Library Library;

enum
{
    Paths_Size = 256
};

// TODO (Matteo): Maybe simplify a bit? I suspect that the full exe path is enough
typedef struct Paths
{
    Char8 buffer[3 * Paths_Size];
    Str base, data, exe_name, lib_name;
} Paths;

typedef struct Platform
{
    /// Virtual memory services
    CfVirtualMemory *vm;
    /// Reserved VM size in bytes
    // TODO (Matteo): Should be a pointer?
    Usize reserved_size;
    /// Committed VM size in bytes
    // TODO (Matteo): Should be a pointer?
    Usize committed_size;

    /// System heap allocator
    CfAllocator heap;
    /// Number of blocks allocated by the heap allocator
    // TODO (Matteo): Should be a pointer?
    Usize heap_blocks;
    // Total size in bytes of the allocation provided by the heap
    // TODO (Matteo): Should be a pointer?
    Usize heap_size;

    /// File system services
    CfFileSystem *fs;

    /// OpenGL API
    GlApi *gl;

    // Dear Imgui state
    Gui *gui;

    /// Return the amount of nanoseconds elapsed since the start of the application
    /// Useful for performance measurement
    Time (*clock)(void);

    /// Common program paths
    Paths *paths;

    /// Dynamic library loading
    Library *(*libLoad)(Str filename);
    void (*libUnload)(Library *lib);
    void *(*libLoadProc)(Library *restrict lib, Cstr restrict name);

} Platform;

//------------------------------------------------------------------------------
// Application interface
//------------------------------------------------------------------------------

typedef struct AppState AppState;
typedef struct FontOptions FontOptions;

// TODO (Matteo): Maybe refactor this a bit

/// Data that can be exchanged between application and platform
typedef struct AppIo
{
    // Inputs
    FontOptions *font_opts;

    // Outputs
    Cstr window_title;
    Rgba32 back_color;
    bool quit;
    bool rebuild_fonts;
    bool continuous_update;
    bool fullscreen;
} AppIo;

#define APP_PROC(name) void name(AppState *app)
#define APP_CREATE_PROC(name) AppState *name(Platform *plat, Cstr argv[], I32 argc)
#define APP_UPDATE_PROC(name) void name(AppState *state, AppIo *io)

typedef APP_PROC((*AppProc));
typedef APP_CREATE_PROC((*AppCreateProc));
typedef APP_UPDATE_PROC((*AppUpdateProc));

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
