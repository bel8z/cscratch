#ifndef APP_H

#include "foundation/platform.h"

typedef struct AppPaths
{
    char base[1024];
    char data[1024];
} AppPaths;

typedef struct AppState AppState;

AppState *appCreate(cfPlatform *plat, AppPaths paths);
void appDestroy(AppState *app);
bool appPrepareUpdate(AppState *app);
void appUpdate(AppState *app);

#define APP_H
#endif
