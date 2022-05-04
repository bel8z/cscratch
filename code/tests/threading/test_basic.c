#include "foundation/core.h"
#include "foundation/threading.h"
#include "foundation/time.h"

#include <stdio.h>
#include <stdlib.h>

//------------------------------------------------------------------------------

typedef struct Platform Platform;

enum
{
    Consumer = 0,
    Producer = 1,
    Count,
};

enum
{
    QueueSize = 32,
    StopSignal = 0,
};

typedef struct Queue
{
    I32 buffer[QueueSize];
    U32 beg;
    U32 len;
    CfMutex lock;
    CfConditionVariable notify;
} Queue;

typedef struct Data
{
    Queue *queue;
} Data;

CF_INTERNAL void
produce(Queue *queue, I32 value)
{
    cfMutexAcquire(&queue->lock);

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
    cfCvSignalOne(&queue->notify);
    cfMutexRelease(&queue->lock);
}

CF_INTERNAL CF_THREAD_FN(producerFn)
{
    Data *d = args;
    Queue *queue = d->queue;

    for (Usize produced_values = 0; produced_values < QueueSize * 2; ++produced_values)
    {
        cfSleep(timeDurationMs(20));

        I32 value = StopSignal;
        while (value == StopSignal)
        {
            value = rand();
        }

        produce(queue, value);
    }

    produce(queue, StopSignal);
}

CF_INTERNAL CF_THREAD_FN(consumerFn)
{
    Data *d = args;
    Queue *queue = d->queue;
    bool go = true;

    cfSleep(timeDurationMs(1000));

    while (go)
    {
        cfMutexAcquire(&queue->lock);
        if (queue->len)
        {
            I32 value = queue->buffer[queue->beg];
            if (value == StopSignal) go = false;

            queue->beg = (queue->beg + 1) % QueueSize;
            --queue->len;

            fprintf(stdout, "Consumed: %d\n", value);
        }
        else
        {
            cfCvWaitMutex(&queue->notify, &queue->lock, timeDurationMs(15));
        }
        cfMutexRelease(&queue->lock);
    }
}

CF_DIAGNOSTIC_PUSH()
CF_DIAGNOSTIC_IGNORE_MSVC(4221)

bool
testBasic(Platform *platform)
{
    CF_UNUSED(platform);

    Queue queue = {0};
    Data data = {.queue = &queue};

    cfMutexInit(&queue.lock);
    cfCvInit(&queue.notify);

    CfThread threads[Count] = {[Producer] = cfThreadCreate(&(CfThreadParms){
                                   .fn = producerFn,
                                   .args = &data,
                                   .debug_name = "Producer thread",
                               }),
                               [Consumer] = cfThreadCreate(&(CfThreadParms){
                                   .fn = consumerFn,
                                   .args = &data,
                                   .debug_name = "Consumer thread",
                               })};

    cfThreadWaitAll(threads, Count, DURATION_INFINITE);
    fflush(stdout);

    return true;
}

CF_DIAGNOSTIC_POP()
