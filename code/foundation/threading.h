#ifndef FOUNDATION_THREADING_H

#include "common.h"

#define THREAD_PROC(name) void name(void *data)

typedef THREAD_PROC((*ThreadProc));

typedef u64 ThreadHandle;

typedef struct ThreadParms
{
    ThreadProc proc;
    void *data;
    char const *name;
    usize stack_size;
} ThreadParms;

typedef struct Threading
{
    ThreadHandle (*thread_create)(ThreadParms const *parms);
    void (*thread_wait)(ThreadHandle thread);

    void (*sleep)(u32 ms);
} Threading;

#define threadCreate(api, thread_proc, ...) \
    api->thread_create(&(ThreadParms){.proc = (thread_proc), __VA_ARGS__})

#define FOUNDATION_THREADING_H
#endif
