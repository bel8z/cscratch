#ifndef CF_PLATFORM_H

#include "allocator.h"
#include "common.h"
#include "vm.h"

// Basic platform API
typedef struct Platform
{
    Vm vm;
    CfAllocator heap;
} Platform;

Platform platform_create();

#define CF_PLATFORM_H
#endif
