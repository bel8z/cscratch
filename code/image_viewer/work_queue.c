#include "work_queue.h"

// Work queue implementation, using the bounded MPMC queue described in
// https://www.1024cores.net/home/lock-free-algorithms/queues/bounded-mpmc-queue

#include "foundation/atom.h"
#include "foundation/atom.inl"
#include "foundation/memory.h"
#include "foundation/threading.h"

static inline Usize
nextPowerOf2(Usize x)
{
    x--;
    x |= x >> 1;
    x |= x >> 2;
    x |= x >> 4;
    x |= x >> 8;
    x |= x >> 16;

#if CF_PTR_SIZE == 8
    x |= x >> 32;
#endif

    return x;
}

//===================================//
// Data

typedef struct WorkQueueCell
{
    // User data
    WorkQueueProc proc;
    void *data;
    // Sequence counter
    AtomUsize sequence;
} WorkQueueCell;

// TODO (Matteo): Better cache line alignment strategy to avoid wasting memory

struct WorkQueue
{
    // TODO (Matteo): Is this padding required?
    CF_CACHELINE_PAD;

    WorkQueueCell *buffer;
    Usize buffer_mask;
    // TODO (Matteo): Should the semaphore be kept in a different cache line from the buffer?
    CfSemaphore semaphore;
    AtomBool stop;

    // NOTE (Matteo): Read and write indices are kept in separate cache lines to avoid false sharing
    CF_CACHELINE_PAD;
    AtomUsize enqueue_pos;
    CF_CACHELINE_PAD;
    AtomUsize dequeue_pos;

    // TODO (Matteo): Is this padding required?
    CF_CACHELINE_PAD;

    CfThread *workers;
    Usize num_workers;
};

//===================================//
// Internals

static bool worqDequeue(WorkQueue *queue, WorkQueueProc *out_proc, void **out_data);

static bool
worqTryWork(WorkQueue *queue)
{
    WorkQueueProc proc;
    void *data;

    if (worqDequeue(queue, &proc, &data))
    {
        proc(data);
        return true;
    }

    return false;
}

static CF_THREAD_PROC(worqTheadProc)
{
    WorkQueue *queue = args;

    while (!atomRead(&queue->stop))
    {
        if (!worqTryWork(queue)) cfSemaWait(&queue->semaphore);
    }
}

//===================================//
// Config/init/shutdown

bool
worqConfig(WorkQueueConfig *config)
{
    Usize buffer_size = config->buffer_size;

    if (buffer_size <= 2) return false;
    if (buffer_size & (buffer_size - 1)) return false;

    if (config->num_workers == 0)
    {
        // TODO (Matteo): Detect number of cores
        return false;
    }

    config->footprint = sizeof(WorkQueue) + buffer_size * sizeof(WorkQueueCell) +
                        config->num_workers * sizeof(CfThread);

    return true;
}

WorkQueue *
worqInit(WorkQueueConfig *config, void *memory)
{
    WorkQueue *queue = memory;

    atomInit(&queue->enqueue_pos, 0);
    atomInit(&queue->dequeue_pos, 0);
    atomInit(&queue->stop, false);
    cfSemaInit(&queue->semaphore, 0);

    Usize buffer_size = config->buffer_size;

    CF_ASSERT(buffer_size >= 2, "Buffer size is too small");
    CF_ASSERT((buffer_size & (buffer_size - 1)) == 0, "Buffer size is not a power of 2");

    queue->buffer_mask = buffer_size - 1;
    queue->buffer = (WorkQueueCell *)(queue + 1);

    for (Usize i = 0; i != buffer_size; i += 1)
    {
        atomWrite(&queue->buffer[i].sequence, i);
    }

    CF_ASSERT(config->num_workers > 0, "Invalid number of workers");

    queue->workers = (CfThread *)((U8 *)queue->buffer + buffer_size * sizeof(*queue->buffer));
    queue->num_workers = config->num_workers;

    return queue;
}

void
worqShutdown(WorkQueue *queue)
{
    atomWrite(&queue->stop, true);
    cfThreadWaitAll(queue->workers, queue->num_workers, DURATION_INFINITE);
}

void
worqStartProcessing(WorkQueue *queue)
{
    atomWrite(&queue->stop, false);
    for (Usize i = 0; i < queue->num_workers; ++i)
    {
        queue->workers[i] = cfThreadStart(worqTheadProc, .args = queue);
    }
}

void
worqStopProcessing(WorkQueue *queue)
{
    atomWrite(&queue->stop, true);
    cfThreadWaitAll(queue->workers, queue->num_workers, DURATION_INFINITE);
}

//===================================//
// Enqueue/dequeue logic

bool
worqEnqueue(WorkQueue *queue, WorkQueueProc proc, void *data)
{
    WorkQueueCell *cell = NULL;
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

    cell->proc = proc;
    cell->data = data;
    atomReleaseFence();
    atomWrite(&cell->sequence, pos + 1);

    cfSemaSignalOne(&queue->semaphore);

    return true;
}

bool
worqDequeue(WorkQueue *queue, WorkQueueProc *out_proc, void **out_data)
{
    WorkQueueCell *cell = NULL;
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

    *out_proc = cell->proc;
    *out_data = cell->data;
    atomReleaseFence();
    atomWrite(&cell->sequence, pos + queue->buffer_mask + 1);

    return true;
}

//===================================//
