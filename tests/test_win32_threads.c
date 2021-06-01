#include "win32/win32_threading.h"

#include "foundation/common.h"

#include "foundation/allocator.h"
#include "foundation/threading.h"

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

static THREAD_PROC(myThreadProc)
{
    Threading *api = args;

    for (i32 i = 10; i >= 0; --i)
    {
        fprintf(stdout, "\r%d", i);
        fflush(stdout);
        api->sleep(1000);
    }
}

static THREAD_PROC(producerProc)
{
    Data *d = args;
    Queue *queue = d->queue;
    Threading *api = d->api;

    while (true)
    {
        api->sleep(1000);
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
        if (api->cvWaitMutex(&queue->notify, &queue->lock, 15))
        {
            i32 value = queue->buffer[queue->beg];

            queue->beg = (queue->beg + 1) % QueueSize;
            --queue->len;

            fprintf(stdout, "Consumed: %d\n", value);
            fflush(stdout);
        }
        api->mutexRelease(&queue->lock);
    }
}

static CF_ALLOCATOR_FUNC(reallocate)
{
    CF_UNUSED(state);
    CF_UNUSED(old_size);

    void *new_memory = NULL;

    if (new_size)
    {
        new_memory = realloc(memory, new_size);
    }
    else if (memory)
    {
        free(memory);
    }

    return new_memory;
}

#pragma warning(push)
#pragma warning(disable : 4221)

int
main(void)
{
    cfAllocator alloc = {.reallocate = reallocate};

    Threading api = {0};
    win32ThreadingInit(&api, &alloc);

    Queue queue = {0};
    Data data = {.api = &api, .queue = &queue};

    api.mutexInit(&queue.lock);

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

    api.threadWaitAll(threads, Count, TIMEOUT_INFINITE);

    return 0;
}

#pragma warning(pop)
