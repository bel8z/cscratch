#pragma once

#include "foundation/common.h"

typedef struct cfThreading cfThreading;

void win32ThreadingInit(cfThreading *threading, cfAllocator *allocator);
