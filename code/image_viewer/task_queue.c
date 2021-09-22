#include "task_queue.h"

// Task system implementation based on the bounded MPMC queue described in
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

typedef struct Task
{
    TaskProc proc;
    void *data;
} Task;

typedef struct TaskQueueCell
{
    Task task;
    AtomUsize sequence;
} TaskQueueCell;

// TODO (Matteo): Better cache line alignment strategy to avoid wasting memory

struct TaskQueue
{
    // TODO (Matteo): Is this padding required?
    CF_CACHELINE_PAD;

    TaskQueueCell *buffer;
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

static bool taskDequeue(TaskQueue *queue, Task *out_task);

static CF_THREAD_PROC(taskThreadProc)
{
    TaskQueue *queue = args;

    while (!atomRead(&queue->stop))
    {
        if (!taskTryWork(queue)) cfSemaWait(&queue->semaphore);
    }
}

//===================================//
// Config/init/shutdown

bool
taskConfig(TaskQueueConfig *config)
{
    Usize buffer_size = config->buffer_size;

    if (buffer_size <= 2) return false;
    if (buffer_size & (buffer_size - 1)) return false;

    if (config->num_workers == 0)
    {
        // TODO (Matteo): Detect number of cores
        return false;
    }

    config->footprint = sizeof(TaskQueue) + buffer_size * sizeof(TaskQueueCell) +
                        config->num_workers * sizeof(CfThread);

    return true;
}

static void
taskClear(TaskQueue *queue)
{
    CF_ASSERT_NOT_NULL(queue->buffer);
    CF_ASSERT(atomRead(&queue->stop), "Cannot flush while running");

    Usize buffer_size = queue->buffer_mask + 1;

    atomWrite(&queue->enqueue_pos, 0);
    atomWrite(&queue->dequeue_pos, 0);

    for (Usize i = 0; i != buffer_size; i += 1)
    {
        atomWrite(&queue->buffer[i].sequence, i);
    }
}

TaskQueue *
taskInit(TaskQueueConfig *config, void *memory)
{
    TaskQueue *queue = memory;

    atomInit(&queue->stop, true);
    cfSemaInit(&queue->semaphore, 0);

    Usize buffer_size = config->buffer_size;

    CF_ASSERT(buffer_size >= 2, "Buffer size is too small");
    CF_ASSERT((buffer_size & (buffer_size - 1)) == 0, "Buffer size is not a power of 2");

    queue->buffer_mask = buffer_size - 1;
    queue->buffer = (TaskQueueCell *)(queue + 1);
    taskClear(queue);

    CF_ASSERT(config->num_workers > 0, "Invalid number of workers");

    queue->workers = (CfThread *)((U8 *)queue->buffer + buffer_size * sizeof(*queue->buffer));
    queue->num_workers = config->num_workers;

    return queue;
}

void
taskShutdown(TaskQueue *queue)
{
    taskStopProcessing(queue, true);
}

void
taskStartProcessing(TaskQueue *queue)
{
    CF_ASSERT(atomRead(&queue->stop), "Queue already started");

    atomWrite(&queue->stop, false);
    for (Usize i = 0; i < queue->num_workers; ++i)
    {
        queue->workers[i] = cfThreadStart(taskThreadProc, .args = queue);
    }
}

void
taskStopProcessing(TaskQueue *queue, bool flush)
{
    CF_ASSERT(!atomRead(&queue->stop), "Queue not running");

    atomWrite(&queue->stop, true);
    cfSemaSignal(&queue->semaphore, queue->num_workers);
    cfThreadWaitAll(queue->workers, queue->num_workers, DURATION_INFINITE);

    if (flush) taskClear(queue);
}

//===================================//
// Enqueue/dequeue logic

bool
taskEnqueue(TaskQueue *queue, TaskProc proc, void *data)
{
    TaskQueueCell *cell = NULL;
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

    cell->task.proc = proc;
    cell->task.data = data;
    atomReleaseFence();
    atomWrite(&cell->sequence, pos + 1);

    cfSemaSignalOne(&queue->semaphore);

    return true;
}

bool
taskDequeue(TaskQueue *queue, Task *out_task)
{
    TaskQueueCell *cell = NULL;
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

    *out_task = cell->task;
    atomReleaseFence();
    atomWrite(&cell->sequence, pos + queue->buffer_mask + 1);

    return true;
}

//===================================//

bool
taskTryWork(TaskQueue *queue)
{
    Task task;

    if (taskDequeue(queue, &task))
    {
        task.proc(task.data);
        return true;
    }

    return false;
}

// bool
// taskIsCompleted(TaskQueue *queue, TaskID id)
// {
//     Usize pos = ~id;
//     TaskQueueCell *cell = queue->buffer + (pos & queue->buffer_mask);
//     return ((pos + 1) == atomRead(&cell->sequence));
// }
