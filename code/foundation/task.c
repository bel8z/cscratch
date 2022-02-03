#include "task.h"

// Task system implementation based on the bounded MPMC queue described in
// https://www.1024cores.net/home/lock-free-algorithms/queues/bounded-mpmc-queue

#include "atom.inl"
#include "error.h"
#include "memory.h"
#include "threading.h"

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

typedef struct
{
    TaskId id;
    TaskFn fn;
    void *data;
    bool canceled;
} Task;

typedef struct
{
    Task task;
    AtomUsize sequence;
} TaskQueueCell;

typedef struct
{
    TaskQueue *queue;
    Task curr_task;
} TaskWorkerSlot;

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

    Usize num_workers;
    CfThread *workers;
    TaskWorkerSlot *worker_slots;
};

//===================================//
// Internals

static bool taskDequeue(TaskQueue *queue, Task *out_task);

static CF_THREAD_FN(taskThreadProc)
{
    TaskWorkerSlot *slot = args;
    TaskQueue *queue = slot->queue;

    while (!atomRead(&queue->stop))
    {
        Task *task = &slot->curr_task;

        if (taskDequeue(queue, task))
        {
            task->fn(task->data, &task->canceled);
        }
        else
        {
            cfSemaWait(&queue->semaphore);
        }
    }
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

static Task *
taskFindInQueue(TaskQueue *queue, TaskId id)
{
    Task *task = NULL;

    Usize pos = ~id;
    TaskQueueCell *cell = queue->buffer + (pos & queue->buffer_mask);
    Usize seq = atomRead(&cell->sequence);

    atomAcquireFence();

    if (seq == pos + 1) task = &cell->task;

    return task;
}

static Task *
taskFindInProgress(TaskQueue *queue, TaskId id)
{
    for (Usize i = 0; i < queue->num_workers; ++i)
    {
        Task *task = &queue->worker_slots[i].curr_task;
        if (task->id == id) return task;
    }

    return NULL;
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
        config->num_workers = cfNumCores();
    }

    config->footprint = sizeof(TaskQueue) + buffer_size * sizeof(TaskQueueCell) +
                        config->num_workers * (sizeof(CfThread) + sizeof(TaskWorkerSlot));

    return true;
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

    Usize workers_offset = buffer_size * sizeof(*queue->buffer);
    Usize slots_offset = workers_offset + config->num_workers * (sizeof(*queue->workers));

    queue->num_workers = config->num_workers;
    queue->workers = (CfThread *)((U8 *)queue->buffer + workers_offset);
    queue->worker_slots = (TaskWorkerSlot *)((U8 *)queue->buffer + slots_offset);

    return queue;
}

void
taskShutdown(TaskQueue *queue)
{
    taskStopProcessing(queue, true);
}

//===================================//
// Start/stop processing

bool
taskStartProcessing(TaskQueue *queue)
{
    if (atomExchange(&queue->stop, false))
    {
        for (Usize i = 0; i < queue->num_workers; ++i)
        {
            TaskWorkerSlot *slot = queue->worker_slots + i;
            slot->queue = queue;
            queue->workers[i] = cfThreadStart(taskThreadProc, .args = slot);
        }

        return true;
    }

    return false;
}

bool
taskStopProcessing(TaskQueue *queue, bool flush)
{
    if (!atomExchange(&queue->stop, true))
    {
        cfSemaSignal(&queue->semaphore, queue->num_workers);
        cfThreadWaitAll(queue->workers, queue->num_workers, DURATION_INFINITE);

        if (flush) taskClear(queue);

        return true;
    }

    return false;
}

//===================================//
// Enqueue/dequeue logic

TaskId
taskEnqueue(TaskQueue *queue, TaskFn fn, void *data)
{
    TaskQueueCell *cell = NULL;
    Usize pos = atomRead(&queue->enqueue_pos);

    for (;;)
    {
        cell = queue->buffer + (pos & queue->buffer_mask);

        Usize seq = atomRead(&cell->sequence);
        atomAcquireFence();

        Isize dif = (Isize)seq - (Isize)pos;

        if (dif < 0) return 0; // Full

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

    cell->task.id = ~pos;
    cell->task.fn = fn;
    cell->task.data = data;
    cell->task.canceled = false;
    atomReleaseFence();
    atomWrite(&cell->sequence, pos + 1);

    cfSemaSignalOne(&queue->semaphore);

    CF_ASSERT(cell->task.id, "This should be always > 0");

    return cell->task.id;
}

static bool
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
// Misc

bool
taskTryWork(TaskQueue *queue)
{
    Task task;

    if (taskDequeue(queue, &task))
    {
        // TODO (Matteo): This task cannot be canceled after it is dequeued
        task.fn(task.data, &task.canceled);
        return true;
    }

    return false;
}

bool
taskCompleted(TaskQueue *queue, TaskId id)
{
    CF_ASSERT(id, "Invalid task ID");

    if (taskFindInQueue(queue, id)) return false;
    if (taskFindInProgress(queue, id)) return false;

    return true;
}

bool
taskCancel(TaskQueue *queue, TaskId id)
{
    CF_ASSERT(id, "Invalid task ID");

    Task *task = taskFindInQueue(queue, id);

    if (!task) task = taskFindInProgress(queue, id);

    if (task)
    {
        task->canceled = true;
        return true;
    }

    return false;
}
