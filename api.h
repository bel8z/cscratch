#ifndef APP_H

#include "foundation/common.h"

#include "foundation/allocator.h"
#include "foundation/fs.h"
#include "foundation/vm.h"

typedef struct cfPlatform cfPlatform;

typedef struct AppState AppState;
typedef struct AppPaths AppPaths;

typedef struct FontOptions FontOptions;

struct cfPlatform
{
    cfVirtualMemory vm;
    cfAllocator heap;
    cfFileSystem fs;
};

typedef enum AppUpdateFlags
{
    AppUpdateFlags_None = 0,

    AppUpdateFlags_RebuildFonts = 1 << 1,

    AppUpdateFlags_All = U32_MAX,
} AppUpdateFlags;

enum
{
    AppPaths_Length = 256,
};

struct AppPaths
{
    char base[AppPaths_Length];
    char data[AppPaths_Length];
};

AppState *appCreate(cfPlatform *plat, AppPaths paths, char *argv[], i32 argc);
void appDestroy(AppState *app);
AppUpdateFlags appUpdate(AppState *app, FontOptions *opts);

#define APP_H
#endif