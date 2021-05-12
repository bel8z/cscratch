#ifndef APP_H

#include "foundation/common.h"

#include "foundation/allocator.h"
#include "foundation/fs.h"
#include "foundation/vm.h"

// Basic platform API
typedef struct cfPlatform
{
    cfVirtualMemory vm;
    cfAllocator heap;
    cfFileSystem fs;
} cfPlatform;

typedef struct FontOptions
{
    i32 tex_glyph_padding;
    f32 rasterizer_multiply;
    // Freetype only
    bool freetype_enabled;
    u32 freetype_flags;
    // Stb only
    i32 oversample_h;
    i32 oversample_v;
} FontOptions;

enum
{
    AppPaths_Length = 256,
};

typedef struct AppPaths
{
    char base[AppPaths_Length];
    char data[AppPaths_Length];
} AppPaths;

typedef enum AppUpdateFlags
{
    AppUpdateFlags_None = 0,

    AppUpdateFlags_RebuildFonts = 1 << 1,

    AppUpdateFlags_All = U32_MAX,
} AppUpdateFlags;

typedef struct AppState AppState;

AppState *appCreate(cfPlatform *plat, AppPaths paths, char *argv[], i32 argc);
void appDestroy(AppState *app);
AppUpdateFlags appUpdate(AppState *app, FontOptions *opts);

#define APP_H
#endif
