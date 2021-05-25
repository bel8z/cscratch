#ifndef APP_H

#include "foundation/common.h"

//------------------------------------------------------------------------------
// Platform interface
//------------------------------------------------------------------------------

typedef struct cfVirtualMemory cfVirtualMemory;
typedef struct cfAllocator cfAllocator;
typedef struct cfFileSystem cfFileSystem;

typedef struct GlApi GlApi;

typedef struct cfPlatform
{
    /// Virtual memory services
    cfVirtualMemory *vm;

    /// System heap allocator
    cfAllocator *heap;
    /// Number of blocks allocated by the heap allocator
    usize heap_blocks;
    // Total size in bytes of the allocation provided by the heap
    usize heap_size;

    /// File system services
    cfFileSystem *fs;

    /// OpenGL API
    GlApi *gl;

    /// Return the amount of nanoseconds elapsed since the start of the application
    /// Useful for performance measurement
    u64 (*clock)(void);

} cfPlatform;

//------------------------------------------------------------------------------
// Application interface
//------------------------------------------------------------------------------

typedef struct AppState AppState;
typedef struct AppPaths AppPaths;

typedef struct FontOptions FontOptions;

typedef enum AppUpdateFlags
{
    AppUpdateFlags_None = 0,

    AppUpdateFlags_RebuildFonts = 1 << 1,

    AppUpdateFlags_All = I32_MAX,
} AppUpdateFlags;

typedef struct AppUpdateResult
{
    AppUpdateFlags flags;
    Rgba32 back_color;
} AppUpdateResult;

enum
{
    AppPaths_Length = 256,
};

struct AppPaths
{
    char base[AppPaths_Length];
    char data[AppPaths_Length];
};

AppState *appCreate(cfPlatform *plat, AppPaths paths, char const *argv[], i32 argc);
void appDestroy(AppState *app);
AppUpdateResult appUpdate(AppState *app, FontOptions *opts);

#define APP_H
#endif
