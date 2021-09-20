#include "foundation/atom.inl"
#include "foundation/threading.h"

#include <stdlib.h>

struct ArEventTest
{
    CfAutoResetEvent *events;
    I32 thread_count;
    I32 iteration_count;
    AtomI32 counter;
    AtomBool success;
} g_test = {0};

static void
kickThreads(I32 except_thread)
{
    for (I32 i = 0; i < g_test.thread_count; ++i)
    {
        if (i != except_thread) cfArEventSignal(g_test.events + i);
    }
}

F32
randF32(void)
{
    return (F32)rand() / (F32)RAND_MAX;
}

CF_THREAD_PROC(threadFun)
{
    I32 thread_num = (I32)(Iptr)args;
    bool is_kicker = (thread_num == 0);

    for (I32 i = 0; i < g_test.iteration_count; i++)
    {
        if (is_kicker)
        {
            atomWrite(&g_test.counter, g_test.thread_count);
            kickThreads(thread_num);
        }
        else
        {
            cfArEventWait(g_test.events + thread_num);
        }

        // Decrement shared counter
        I32 previous = atomFetchDec(&g_test.counter);
        if (previous < 1) atomWrite(&g_test.success, false);

        // Last one to decrement becomes the kicker next time
        is_kicker = (previous == 1);

        // Do a random amount of work in the range [0, 10) units, biased towards low numbers.
        F32 f = randF32();
        I32 workUnits = (I32)(f * f * 10);
        for (I32 j = 1; j < workUnits; j++) randF32(); // Do one work unit
    }
}

int
main(void)
{
    g_test.thread_count = 4;
    g_test.iteration_count = 100; // 1000000;
    g_test.events = calloc((Usize)g_test.thread_count, sizeof(CfAutoResetEvent));
    atomInit(&g_test.counter, 0);
    atomInit(&g_test.success, true);

    CfThread *threads = calloc((Usize)g_test.thread_count, sizeof(*threads));
    for (I32 i = 0; i < g_test.thread_count; ++i)
    {
        threads[i] = cfThreadStart(threadFun, .args = (void *)(Iptr)i);
    }

    for (I32 i = 0; i < g_test.thread_count; ++i)
    {
        cfThreadWait(threads[i], DURATION_INFINITE);
    }

    free(g_test.events);
    free(threads);

    return atomRead(&g_test.success) ? 0 : -1;
}
