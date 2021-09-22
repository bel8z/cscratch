#include "platform.h"

#include "foundation/atom.h"
#include "foundation/atom.inl"
#include "foundation/memory.h"
#include "foundation/threading.h"
#include "foundation/time.h"

typedef struct Platform Platform;

typedef struct QueueCell
{
    AtomUsize sequence;
    Usize data;
} QueueCell;

typedef struct MpmcQueue
{
    CF_CACHELINE_PAD;

    QueueCell *buffer;
    Usize buffer_mask;

    CF_CACHELINE_PAD;

    AtomUsize enqueue_pos;

    CF_CACHELINE_PAD;

    AtomUsize dequeue_pos;

    CF_CACHELINE_PAD;
} MpmcQueue;

void
mpmcInit(MpmcQueue *queue, Usize buffer_size, MemAllocator alloc)
{
    queue->buffer = memAllocArray(alloc, QueueCell, buffer_size);
    queue->buffer_mask = buffer_size - 1;

    CF_ASSERT(buffer_size >= 2, "Buffer size is too small");
    CF_ASSERT((buffer_size & (buffer_size - 1)) == 0, "Buffer size is not a power of 2");

    for (Usize i = 0; i != buffer_size; i += 1)
    {
        atomWrite(&queue->buffer[i].sequence, i);
    }

    atomWrite(&queue->enqueue_pos, 0);
    atomWrite(&queue->dequeue_pos, 0);
}

void
mpmcShutdown(MpmcQueue *queue, MemAllocator alloc)
{
    memFreeArray(alloc, queue->buffer, queue->buffer_mask + 1);
}

bool
mpmcEnqueue(MpmcQueue *queue, Usize data)
{
    QueueCell *cell = NULL;
    Usize pos = atomRead(&queue->enqueue_pos);

    for (;;)
    {
        cell = queue->buffer + (pos & queue->buffer_mask);

        Usize seq = atomRead(&cell->sequence);
        atomAcquireFence();

        Isize dif = (Isize)seq - (Isize)pos;

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

bool
mpmcDequeue(MpmcQueue *queue, Usize *data)
{
    QueueCell *cell = NULL;
    Usize pos = atomRead(&queue->dequeue_pos);

    for (;;)
    {
        cell = queue->buffer + (pos & queue->buffer_mask);

        Usize seq = atomRead(&cell->sequence);
        atomAcquireFence();

        Isize dif = (Isize)seq - (Isize)(pos + 1);

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

#include <immintrin.h>
#include <stdlib.h>
#include <time.h>

#define THREAD_COUNT 4
#define BATCH_SIZE 1
#define ITER_COUNT 2000000
AtomBool g_start;

CF_THREAD_PROC(thread_func)
{
    MpmcQueue *queue = args;
    Usize data;

    U32 id = cfCurrentThreadId();

    srand((U32)time(0) + id);
    Usize pause = (Usize)rand() % 1000;

    while (!atomRead(&g_start)) cfYield();

    for (Usize i = 0; i != pause; i += 1) _mm_pause();

    for (Usize iter = 0; iter != ITER_COUNT; ++iter)
    {
        for (Usize i = 0; i != BATCH_SIZE; i += 1)
        {
            while (!mpmcEnqueue(queue, i)) cfYield();
        }
        for (Usize i = 0; i != BATCH_SIZE; i += 1)
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
    for (Usize i = 0; i != THREAD_COUNT; ++i)
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

    return true;
}
