#include "win32/win32_threading.h"

#include "foundation/common.h"

#include "foundation/allocator.h"
#include "foundation/threading.h"

#include "std_allocator.h"

#include <stdio.h>
#include <stdlib.h>

//------------------------------------------------------------------------------

enum
{
    Consumer = 0,
    Producer = 1,
    Count,
};

enum
{
    QueueSize = 1024
};

typedef struct Queue
{
    i32 buffer[QueueSize];
    u32 beg;
    u32 len;
    Mutex lock;
    ConditionVariable notify;
} Queue;

typedef struct Data
{
    Threading *api;
    Queue *queue;
} Data;

static THREAD_PROC(producerProc)
{
    Data *d = args;
    Queue *queue = d->queue;
    Threading *api = d->api;

    while (true)
    {
        api->sleep(TIME_MS(1000));
        api->mutexAcquire(&queue->lock);

        i32 value = rand();

        fprintf(stdout, "Produced: %d", value);

        queue->buffer[(queue->beg + queue->len) % QueueSize] = value;

        if (queue->len == QueueSize)
        {
            // Full buffer, overwrite front
            queue->beg = (queue->beg + 1) % QueueSize;
            fprintf(stdout, "\tQueue full");
        }
        else
        {
            ++queue->len;
        }

        fprintf(stdout, "\n");
        fflush(stdout);
        api->cvSignalOne(&queue->notify);
        api->mutexRelease(&queue->lock);
    }
}

static THREAD_PROC(consumerProc)
{
    Data *d = args;
    Queue *queue = d->queue;
    Threading *api = d->api;

    while (true)
    {
        api->mutexAcquire(&queue->lock);
        if (queue->len)
        {
            i32 value = queue->buffer[queue->beg];

            queue->beg = (queue->beg + 1) % QueueSize;
            --queue->len;

            fprintf(stdout, "Consumed: %d\n", value);
            fflush(stdout);
        }
        else
        {
            api->cvWaitMutex(&queue->notify, &queue->lock, TIME_MS(15));
        }
        api->mutexRelease(&queue->lock);
    }
}

#pragma warning(push)
#pragma warning(disable : 4221)

int
main(void)
{
    cfAllocator alloc = stdAllocator();

    Threading api = {0};
    win32ThreadingInit(&api, &alloc);

    Queue queue = {0};
    Data data = {.api = &api, .queue = &queue};

    api.mutexInit(&queue.lock);
    api.cvInit(&queue.notify);

    Thread threads[Count] = {[Producer] = api.threadCreate(&(ThreadParms){
                                 .proc = producerProc,
                                 .args = &data,
                                 .debug_name = "Producer thread",
                             }),
                             [Consumer] = api.threadCreate(&(ThreadParms){
                                 .proc = consumerProc,
                                 .args = &data,
                                 .debug_name = "Consumer thread",
                             })};

    api.threadWaitAll(threads, Count, TIME_INFINITE);

    return 0;
}

#pragma warning(pop)
