#include "work_queue.h"

#include "foundation/atom.h"
#include "foundation/atom.inl"
#include "foundation/memory.h"
#include "foundation/threading.h"

struct WorkQueue
{
    CfThread worker;
    CfSemaphore wake;

    Usize size;
    AtomUsize read, write;
    WorkItem *buffer;
};

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

static bool
wkTryDequeue(WorkQueue *queue, WorkItem *item)
{
    Usize read = atomRead(&queue->read);

    if (atomRead(&queue->write) - read > 0)
    {
        atomAcquireFence();

        *item = queue->buffer[read & (queue->size - 1)];
        atomReleaseFence();
        atomWrite(&queue->read, read + 1);

        return true;
    }

    return false;
}

static CF_THREAD_PROC(wkWorkerProc)
{
    WorkQueue *queue = args;

    for (;;)
    {
        WorkItem item;
        if (wkTryDequeue(queue, &item))
        {
            item.proc(item.data);
        }
        else
        {
            cfSemaWait(&queue->wake);
        }
    }
}

WorkQueueConfig
wkConfig(Usize buffer_size)
{
    buffer_size = nextPowerOf2(buffer_size);
    return (WorkQueueConfig){
        .buffer_size = buffer_size,
        .footprint = sizeof(WorkQueue) + sizeof(WorkItem) * buffer_size,
    };
}

WorkQueue *
wkAllocate(WorkQueueConfig config)
{
    WorkQueue *queue = config.memory;

    queue->buffer = (WorkItem *)(queue + 1);
    queue->size = config.buffer_size;
    atomWrite(&queue->read, 0);
    atomWrite(&queue->write, 0);
    queue->worker = cfThreadStart(wkWorkerProc);
    cfSemaInit(&queue->wake, 0);

    return queue;
}

void
wkTerminate(WorkQueue *queue)
{
    // BUG (Matteo): This stalls
    cfThreadWait(queue->worker, DURATION_INFINITE);
    queue->size = 0;
}

bool
wkIsFull(WorkQueue *queue)
{
    Usize write = atomRead(&queue->write);
    Usize read = atomRead(&queue->read);
    atomAcquireFence();
    return write < read + queue->size;
}

bool
wkIsEmpty(WorkQueue *queue)
{
    Usize write = atomRead(&queue->write);
    Usize read = atomRead(&queue->read);
    atomAcquireFence();
    return (write - read) == 0;
}

bool
wkPush(WorkQueue *queue, WorkItem item)
{
    Usize write = atomRead(&queue->write);

    if (write < atomRead(&queue->read) + queue->size)
    {
        atomAcquireFence();

        queue->buffer[write & (queue->size - 1)] = item;
        atomReleaseFence();
        atomWrite(&queue->write, write + 1);

        cfSemaSignalOne(&queue->wake);

        return true;
    }

    return false;
}
