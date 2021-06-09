#ifndef APP_H

#include "foundation/common.h"

//------------------------------------------------------------------------------
// Platform interface
//------------------------------------------------------------------------------

typedef struct cfVirtualMemory cfVirtualMemory;
typedef struct cfAllocator cfAllocator;
typedef struct cfFileSystem cfFileSystem;
typedef struct Threading Threading;

typedef struct GlApi GlApi;

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
    char dll_name[Paths_Size];
} Paths;

typedef struct cfPlatform
{
    /// Virtual memory services
    cfVirtualMemory *vm;
    /// Reserved VM size in bytes
    Usize reserved_size; // TODO (Matteo): Should be a pointer?
    /// Committed VM size in bytes
    Usize committed_size; // TODO (Matteo): Should be a pointer?

    /// System heap allocator
    cfAllocator *heap;
    /// Number of blocks allocated by the heap allocator
    Usize heap_blocks; // TODO (Matteo): Should be a pointer?
    // Total size in bytes of the allocation provided by the heap
    Usize heap_size; // TODO (Matteo): Should be a pointer?

    /// File system services
    cfFileSystem *fs;

    /// Threading API
    Threading *threading;

    /// OpenGL API
    GlApi *gl;

    /// Return the amount of nanoseconds elapsed since the start of the application
    /// Useful for performance measurement
    Time (*clock)(void);

    /// Common program paths
    Paths *paths;

    /// Dynamic library loading
    void *(*libLoad)(char const *filename);
    void *(*libLoadProc)(void *restrict lib, char const *restrict name);

} cfPlatform;

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

#define APP_CREATE(name) AppState *name(cfPlatform *plat, char const *argv[], I32 argc)
#define APP_DESTROY(name) void name(AppState *app)
#define APP_UPDATE(name) AppUpdateResult name(AppState *app, FontOptions *opts)

typedef struct AppApi
{
    APP_CREATE((*create));
    APP_DESTROY((*destroy));
    APP_UPDATE((*update));
} AppApi;

#if CF_OS_WIN32
#define APP_API __declspec(dllexport)
#else
#define APP_API
#endif

APP_API void appApiInit(AppApi *api);

APP_CREATE(appCreate);
APP_DESTROY(appDestroy);
APP_UPDATE(appUpdate);

#define APP_H
#endif
