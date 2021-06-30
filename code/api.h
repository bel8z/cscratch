#pragma once

#include "foundation/common.h"

//------------------------------------------------------------------------------
// Platform interface
//------------------------------------------------------------------------------

// Foundation interfaces
typedef struct cfVirtualMemory cfVirtualMemory;
typedef struct cfAllocator cfAllocator;
typedef struct cfFileSystem cfFileSystem;
typedef struct cfThreading cfThreading;

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
    char base[Paths_Size];
    char data[Paths_Size];
    char exe_name[Paths_Size];
    char lib_name[Paths_Size];
} Paths;

typedef struct Platform
{
    /// Virtual memory services
    cfVirtualMemory *vm;
    /// Reserved VM size in bytes
    // TODO (Matteo): Should be a pointer?
    Usize reserved_size;
    /// Committed VM size in bytes
    // TODO (Matteo): Should be a pointer?
    Usize committed_size;

    /// System heap allocator
    cfAllocator *heap;
    /// Number of blocks allocated by the heap allocator
    // TODO (Matteo): Should be a pointer?
    Usize heap_blocks;
    // Total size in bytes of the allocation provided by the heap
    // TODO (Matteo): Should be a pointer?
    Usize heap_size;

    /// File system services
    cfFileSystem *fs;

    /// Threading API
    cfThreading *threading;

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
    Library *(*libLoad)(char const *filename);
    void (*libUnload)(Library *lib);
    void *(*libLoadProc)(Library *restrict lib, char const *restrict name);

} Platform;

//------------------------------------------------------------------------------
// Application interface
//------------------------------------------------------------------------------

typedef struct AppState AppState;

typedef struct FontOptions FontOptions;

typedef enum AppUpdateFlags
{
    AppUpdateFlags_None = 0,

    AppUpdateFlags_RebuildFonts = 1 << 1,
    AppUpdateFlags_Quit = 1 << 2,

    AppUpdateFlags_All = T_MAX(I32),
} AppUpdateFlags;

typedef struct AppUpdateResult
{
    AppUpdateFlags flags;
    Rgba32 back_color;
} AppUpdateResult;

#define APP_PROC(name) void name(AppState *app)
#define APP_CREATE_PROC(name) AppState *name(Platform *plat, char const *argv[], I32 argc)
#define APP_UPDATE_PROC(name) AppUpdateResult name(AppState *app, FontOptions *opts)

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
