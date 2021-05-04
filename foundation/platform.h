#ifndef CF_PLATFORM_H

#include "common.h"

#include "allocator.h"
#include "fs.h"
#include "vm.h"

// Basic platform API
typedef struct cfPlatform
{
    cfVirtualMemory vm;
    cfAllocator heap;
    cfFileSystem fs;
} cfPlatform;

cfPlatform cfPlatformCreate();
void cfPlatformShutdown(cfPlatform *platform);

#define CF_PLATFORM_H
#endif
