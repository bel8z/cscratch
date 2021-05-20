#ifndef APP_H

#include "foundation/common.h"

//------------------------------------------------------------------------------
// Platform interface
//------------------------------------------------------------------------------

typedef struct cfVirtualMemory cfVirtualMemory;
typedef struct cfAllocator cfAllocator;
typedef struct cfFileSystem cfFileSystem;

typedef struct GlApi GlApi;

typedef struct TimePoint
{
    u64 opaque;
} TimePoint;

typedef struct Time
{
    TimePoint (*now)(void);
} Time;

typedef struct cfPlatform
{
    cfVirtualMemory *vm;
    cfAllocator *heap;
    cfFileSystem *fs;

    GlApi *gl;

    Time *time;

    usize heap_blocks;
    usize heap_size;

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
