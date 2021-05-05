#ifndef APP_H

#include "foundation/common.h"

typedef struct cfPlatform cfPlatform;

enum
{
    AppPaths_Length = 256,
};

typedef struct AppPaths
{
    char base[AppPaths_Length];
    char data[AppPaths_Length];
} AppPaths;

typedef struct AppState AppState;

AppState *appCreate(cfPlatform *plat, AppPaths paths);
void appDestroy(AppState *app);
bool appPrepareUpdate(AppState *app);
void appUpdate(AppState *app);

#define APP_H
#endif
