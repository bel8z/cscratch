#include "win32/win32_threading.h"

#include "foundation/common.h"
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
    I32 buffer[QueueSize];
    U32 beg;
    U32 len;
    Mutex lock;
    ConditionVariable notify;
} Queue;

typedef struct Data
{
    cfThreading *api;
    Queue *queue;
} Data;

static CF_THREAD_PROC(producerProc)
{
    Data *d = args;
    Queue *queue = d->queue;
    cfThreading *api = d->api;

    while (true)
    {
        api->sleep(TIME_MS(1000));
        api->mutexAcquire(&queue->lock);

        I32 value = rand();

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

static CF_THREAD_PROC(consumerProc)
{
    Data *d = args;
    Queue *queue = d->queue;
    cfThreading *api = d->api;

    while (true)
    {
        api->mutexAcquire(&queue->lock);
        if (queue->len)
        {
            I32 value = queue->buffer[queue->beg];

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
    cfThreading api = {0};
    win32ThreadingInit(&api);

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
