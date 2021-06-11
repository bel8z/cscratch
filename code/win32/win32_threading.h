#pragma once

#include "foundation/common.h"

typedef struct Threading Threading;

void win32ThreadingInit(Threading *threading, cfAllocator *allocator);
