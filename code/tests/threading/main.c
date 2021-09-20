#include "benaphore.h"

#define THREAD_COUNT 4

typedef struct TestState
{
    Benaphore mutex;
    I32 iteration_count;
    I32 value;
} TestState;

CF_THREAD_PROC(threadFun)
{
    TestState *test = args;

    for (I32 i = 0; i < test->iteration_count; ++i)
    {
        benaLock(&test->mutex);
        test->value++;
        benaRelease(&test->mutex);
    }
}

int
main(void)
{
    TestState test = {.iteration_count = 400000};
    benaInit(&test.mutex);

    CfThread threads[THREAD_COUNT] = {0};

    for (I32 i = 0; i < THREAD_COUNT; ++i)
    {
        threads[i] = cfThreadStart(threadFun, .args = &test);
    }

    for (I32 i = 0; i < THREAD_COUNT; ++i)
    {
        cfThreadWait(threads[i], DURATION_INFINITE);
    }

    if (test.value == THREAD_COUNT * test.iteration_count) return 0;

    return -1;
}
