#ifndef FOUNDATION_THREADING_H

#include "common.h"

#define THREAD_PROC(name) void name(void *data)

typedef THREAD_PROC((*ThreadProc));

typedef struct Thread
{
    u64 opaque;
} Thread;

typedef struct Threading
{
    Thread (*thread_create)(ThreadProc proc, void *data, u64 stack_size);
    void (*thread_destroy)(Thread *thread);
} Threading;

#define FOUNDATION_THREADING_H
#endif
