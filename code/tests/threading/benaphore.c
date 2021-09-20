#include "foundation/atom.inl"
#include "foundation/threading.h"

typedef struct Benaphore
{
    CfSemaphore sema;
    AtomI32 contention_count;
} Benaphore;

void
benaInit(Benaphore *bena)
{
    cfSemaInit(&bena->sema, 0);
    atomInit(&bena->contention_count, 0);
}

void
benaLock(Benaphore *bena)
{
    if (atomFetchInc(&bena->contention_count) > 0)
    {
        atomAcquireFence();
        cfSemaWait(&bena->sema);
    }
}

bool
benaTryLock(Benaphore *bena)
{
    if (atomRead(&bena->contention_count) != 0) return false;

    if (atomCompareExchange(&bena->contention_count, 0, 1) == 0)
    {
        atomAcquireFence();
        return true;
    }

    return false;
}

void
benaRelease(Benaphore *bena)
{
    atomReleaseFence();
    I32 old_count = atomFetchDec(&bena->contention_count);

    CF_ASSERT(old_count > 0, "");

    if (old_count > 1)
    {
        cfSemaSignalOne(&bena->sema);
    }
}

#define THREAD_COUNT 4
I32 g_iteration_count = 400000;
I32 g_value = 0;
Benaphore g_mutex = {0};

CF_THREAD_PROC(threadFun)
{
    CF_UNUSED(args);

    for (I32 i = 0; i < g_iteration_count; ++i)
    {
        benaLock(&g_mutex);
        g_value++;
        benaRelease(&g_mutex);
    }
}

int
main(void)
{
    benaInit(&g_mutex);

    CfThread threads[THREAD_COUNT] = {0};

    for (I32 i = 0; i < THREAD_COUNT; ++i)
    {
        threads[i] = cfThreadStart(threadFun);
    }

    for (I32 i = 0; i < THREAD_COUNT; ++i)
    {
        cfThreadWait(threads[i], DURATION_INFINITE);
    }

    if (g_value == THREAD_COUNT * g_iteration_count) return 0;

    return -1;
}
