#include "foundation/core.h"
#include "foundation/threading.h"
#include "foundation/time.h"

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
    CfMutex lock;
    CfConditionVariable notify;
} Queue;

typedef struct Data
{
    Queue *queue;
} Data;

static CF_THREAD_FN(producerProc)
{
    Data *d = args;
    Queue *queue = d->queue;

    while (true)
    {
        cfSleep(timeDurationMs(1000));
        cfMutexAcquire(&queue->lock);

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
        cfCvSignalOne(&queue->notify);
        cfMutexRelease(&queue->lock);
    }
}

static CF_THREAD_FN(consumerProc)
{
    Data *d = args;
    Queue *queue = d->queue;

    while (true)
    {
        cfMutexAcquire(&queue->lock);
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
            cfCvWaitMutex(&queue->notify, &queue->lock, timeDurationMs(15));
        }
        cfMutexRelease(&queue->lock);
    }
}

CF_DIAGNOSTIC_PUSH()
CF_DIAGNOSTIC_IGNORE_MSVC(4221)

I32
platformMain(Platform *platform, CommandLine *cmd_line)
{
    Queue queue = {0};
    Data data = {.queue = &queue};

    cfMutexInit(&queue.lock);
    cfCvInit(&queue.notify);

    CfThread threads[Count] = {[Producer] = cfThreadCreate(&(CfThreadParms){
                                   .proc = producerProc,
                                   .args = &data,
                                   .debug_name = "Producer thread",
                               }),
                               [Consumer] = cfThreadCreate(&(CfThreadParms){
                                   .proc = consumerProc,
                                   .args = &data,
                                   .debug_name = "Consumer thread",
                               })};

    cfThreadWaitAll(threads, Count, DURATION_INFINITE);

    return 0;
}

CF_DIAGNOSTIC_POP()
