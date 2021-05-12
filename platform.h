#ifndef CF_PLATFORM_H

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

cfPlatform cfPlatformCreate();
void cfPlatformShutdown(cfPlatform *platform);

#define CF_PLATFORM_H
#endif
