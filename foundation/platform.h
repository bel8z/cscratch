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

cfPlatform cf_platform_create();

#define CF_PLATFORM_H
#endif
