#include "auto_reset_event.h"

#include "foundation/atom.inl"
#include "foundation/threading.h"

#include <stdlib.h>

typedef struct Platform Platform;

#define THREAD_COUNT 4
#define ITERATION_COUNT 10 // 10000

typedef struct TestArEventState
{
    CfAutoResetEvent events[THREAD_COUNT];
    AtomI32 counter;
    AtomBool success;
} TestArEventState;

typedef struct ThreadData
{
    TestArEventState *test;
    I32 id;
} ThreadData;

F32
randF32(void)
{
    return (F32)rand() / (F32)RAND_MAX;
}

static CF_THREAD_FN(testArEventWork)
{
    ThreadData *data = args;
    TestArEventState *test = data->test;

    bool is_kicker = (data->id == 0);

    for (I32 iter = 0; iter < ITERATION_COUNT; iter++)
    {
        if (is_kicker)
        {
            atomWrite(&test->counter, THREAD_COUNT);
            for (I32 thread = 0; thread < THREAD_COUNT; ++thread)
            {
                if (thread != data->id) cfArEventSignal(test->events + thread);
            }
        }
        else
        {
            cfArEventWait(test->events + data->id);
        }

        // Decrement shared counter
        I32 previous = atomFetchDec(&test->counter);
        if (previous < 1) atomWrite(&test->success, false);

        // Last one to decrement becomes the kicker next time
        is_kicker = (previous == 1);

        // Do a random amount of work in the range [0, 10) units, biased towards low numbers.
        F32 f = randF32();
        I32 workUnits = (I32)(f * f * 10);
        for (I32 j = 1; j < workUnits; j++)
        {
            randF32(); // Do one work unit
        }
    }
}

bool
testAutoResetEvent(Platform *platform)
{
    CF_UNUSED(platform);

    CfThread threads[THREAD_COUNT] = {0};
    ThreadData thread_data[THREAD_COUNT] = {0};

    TestArEventState test = {0};
    atomInit(&test.counter, 0);
    atomInit(&test.success, true);

    for (I32 i = 0; i < THREAD_COUNT; ++i)
    {
        thread_data[i].id = i;
        thread_data[i].test = &test;
        threads[i] = cfThreadStart(testArEventWork, .args = thread_data + i);
    }

    for (I32 i = 0; i < THREAD_COUNT; ++i)
    {
        cfThreadWait(threads[i], DURATION_INFINITE);
    }

    return atomRead(&test.success);
}
