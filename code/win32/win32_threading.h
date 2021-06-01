#ifndef _WIN32_THREADING_H_

#include "foundation/common.h"

typedef struct Threading Threading;

void win32ThreadingInit(Threading *threading, cfAllocator *allocator);

#define _WIN32_THREADING_H_
#endif
