#include "foundation/core.h"

#include "foundation/atom.h"
#include "foundation/atom.inl"
#include "foundation/memory.h"
#include "foundation/threading.h"
#include "foundation/time.h"

#include "platform.h"

// TODO (Matteo): Replace with platform API
#include <stdio.h>
#include <stdlib.h>

#include <immintrin.h>
#include <time.h>

typedef struct QueueCell
{
    AtomUsize sequence;
    Size data;
} QueueCell;

typedef struct MpmcQueue
{
    CF_CACHELINE_PAD;

    QueueCell *buffer;
    Size buffer_mask;

    CF_CACHELINE_PAD;

    AtomUsize enqueue_pos;

    CF_CACHELINE_PAD;

    AtomUsize dequeue_pos;

    CF_CACHELINE_PAD;
} MpmcQueue;

static void
mpmcInit(MpmcQueue *queue, Size buffer_size, MemAllocator alloc)
{
    queue->buffer = memAllocArray(alloc, QueueCell, buffer_size);
    queue->buffer_mask = buffer_size - 1;

    CF_ASSERT(buffer_size >= 2, "Buffer size is too small");
    CF_ASSERT((buffer_size & (buffer_size - 1)) == 0, "Buffer size is not a power of 2");

    for (Size i = 0; i != buffer_size; i += 1)
    {
        atomWrite(&queue->buffer[i].sequence, i);
    }

    atomWrite(&queue->enqueue_pos, 0);
    atomWrite(&queue->dequeue_pos, 0);
}

static void
mpmcShutdown(MpmcQueue *queue, MemAllocator alloc)
{
    memFreeArray(alloc, queue->buffer, queue->buffer_mask + 1);
}

static bool
mpmcEnqueue(MpmcQueue *queue, Size data)
{
    QueueCell *cell = NULL;
    Size pos = atomRead(&queue->enqueue_pos);

    for (;;)
    {
        cell = queue->buffer + (pos & queue->buffer_mask);

        Size seq = atomRead(&cell->sequence);
        atomAcquireFence();

        Offset dif = (Offset)seq - (Offset)pos;

        if (dif < 0) return false; // Full

        if (dif > 0)
        {
            pos = atomRead(&queue->enqueue_pos);
        }
        else if (atomCompareExchangeWeak(&queue->enqueue_pos, &pos, pos + 1))
        {
            break;
        }
    }

    CF_ASSERT_NOT_NULL(cell);

    cell->data = data;
    atomReleaseFence();
    atomWrite(&cell->sequence, pos + 1);

    return true;
}

static bool
mpmcDequeue(MpmcQueue *queue, Size *data)
{
    QueueCell *cell = NULL;
    Size pos = atomRead(&queue->dequeue_pos);

    for (;;)
    {
        cell = queue->buffer + (pos & queue->buffer_mask);

        Size seq = atomRead(&cell->sequence);
        atomAcquireFence();

        Offset dif = (Offset)seq - (Offset)(pos + 1);

        if (dif < 0) return false; // Empty

        if (dif > 0)
        {
            pos = atomRead(&queue->dequeue_pos);
        }
        else if (atomCompareExchangeWeak(&queue->dequeue_pos, &pos, pos + 1))
        {
            break;
        }
    }

    CF_ASSERT_NOT_NULL(cell);

    *data = cell->data;
    atomReleaseFence();
    atomWrite(&cell->sequence, pos + queue->buffer_mask + 1);

    return true;
}

#define THREAD_COUNT 4
#define BATCH_SIZE 1
#define ITER_COUNT 2000000
static AtomBool g_start;

static
CF_THREAD_FN(thread_func)
{
    MpmcQueue *queue = args;
    Size data;

    U32 id = cfCurrentThreadId();

    srand((U32)time(0) + id);
    Size pause = (Size)rand() % 1000;

    while (!atomRead(&g_start)) cfYield();

    for (Size i = 0; i != pause; i += 1) _mm_pause();

    for (Size iter = 0; iter != ITER_COUNT; ++iter)
    {
        for (Size i = 0; i != BATCH_SIZE; i += 1)
        {
            while (!mpmcEnqueue(queue, i)) cfYield();
        }
        for (Size i = 0; i != BATCH_SIZE; i += 1)
        {
            while (!mpmcDequeue(queue, &data)) cfYield();
        }
    }
}

bool
testMpmcQueue(Platform *platform)
{
    MemAllocator alloc = platform->heap;
    MpmcQueue queue = {0};

    mpmcInit(&queue, 1024, alloc);

    atomInit(&g_start, false);

    CfThread threads[THREAD_COUNT];
    for (Size i = 0; i != THREAD_COUNT; ++i)
    {
        threads[i] = cfThreadStart(thread_func, .args = &queue);
    }

    cfSleep(timeDurationMs(1));

    U64 start = __rdtsc();
    atomWrite(&g_start, true);

    cfThreadWaitAll(threads, THREAD_COUNT, DURATION_INFINITE);

    U64 end = __rdtsc();
    U64 time = end - start;

    printf("cycles/op=%llu\n", time / (BATCH_SIZE * ITER_COUNT * 2 * THREAD_COUNT));

    mpmcShutdown(&queue, alloc);

    return true;
}
