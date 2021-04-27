#ifndef CF_PLATFORM_H

#include "allocator.h"
#include "common.h"
#include "vm.h"

// Basic platform API
typedef struct cfPlatform
{
    cfVirtualMemory vm;
    cfAllocator heap;
} cfPlatform;

cfPlatform cfPlatformCreate();
void cfPlatformShutdown(cfPlatform *platform);

#define CF_PLATFORM_H
#endif
