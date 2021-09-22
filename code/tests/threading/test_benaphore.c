#include "benaphore.h"

#include "foundation/atom.inl"
#include "foundation/threading.h"

typedef struct Platform Platform;

#define THREAD_COUNT 4

typedef struct TestBenaphoreState
{
    Benaphore mutex;
    I32 iteration_count;
    I32 value;
} TestBenaphoreState;

static CF_THREAD_PROC(testBenaphoreWork)
{
    TestBenaphoreState *test = args;

    for (I32 i = 0; i < test->iteration_count; ++i)
    {
        benaLock(&test->mutex);
        test->value++;
        benaRelease(&test->mutex);
    }
}

bool
testBenaphore(Platform *platform)
{
    CF_UNUSED(platform);

    TestBenaphoreState test = {.iteration_count = 400000};
    benaInit(&test.mutex);

    CfThread threads[THREAD_COUNT] = {0};

    for (I32 i = 0; i < THREAD_COUNT; ++i)
    {
        threads[i] = cfThreadStart(testBenaphoreWork, .args = &test);
    }

    for (I32 i = 0; i < THREAD_COUNT; ++i)
    {
        cfThreadWait(threads[i], DURATION_INFINITE);
    }

    return (test.value == THREAD_COUNT * test.iteration_count);
}
