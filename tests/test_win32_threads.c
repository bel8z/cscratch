#include "win32.h"

#include "foundation/common.h"
#include "foundation/threading.h"

#include <stdio.h>
#include <stdlib.h>

#include "win32/win32_threading.c"

static Threading api = {
    .threadCreate = win32ThreadCreate,
    .threadDestroy = win32ThreadDestroy,
    .threadIsRunning = win32ThreadIsRunning,
    .threadWait = win32ThreadWait,
    .threadWaitAll = win32ThreadWaitAll,
    .mutexInit = win32MutexInit,
    .mutexShutdown = win32MutexShutdown,
    .mutexTryAcquire = win32MutexTryAcquire,
    .mutexAcquire = win32MutexAcquire,
    .mutexRelease = win32MutexRelease,
    .rwInit = win32RwInit,
    .rwShutdown = win32RwShutdown,
    .rwTryLockReader = win32RwTryLockReader,
    .rwTryLockWriter = win32RwTryLockWriter,
    .rwLockReader = win32RwLockReader,
    .rwLockWriter = win32RwLockWriter,
    .rwUnlockReader = win32RwUnlockReader,
    .rwUnlockWriter = win32RwUnlockWriter,
    .cvInit = win32CvInit,
    .cvShutdown = win32CvShutdown,
    .cvWaitMutex = win32CvWaitMutex,
    .cvWaitRwLock = win32CvWaitRwLock,
    .cvSignalOne = win32CvSignalOne,
    .cvSignalAll = win32CvSignalAll,
};

//------------------------------------------------------------------------------

#pragma warning(push)
#pragma warning(disable : 4221)

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

static THREAD_PROC(myThreadProc)
{
    Queue *queue = data;

    CF_UNUSED(queue);

    for (i32 i = 10; i >= 0; --i)
    {
        fprintf(stdout, "\r%d", i);
        fflush(stdout);
        Sleep(1000);
    }
}

static THREAD_PROC(producerProc)
{
    Queue *queue = data;

    while (true)
    {
        Sleep(1000);
        api.mutexAcquire(&queue->lock);

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

        api.cvSignalOne(&queue->notify);
        api.mutexRelease(&queue->lock);
    }
}

static THREAD_PROC(consumerProc)
{
    Queue *queue = data;

    while (true)
    {
        api.mutexAcquire(&queue->lock);
        if (api.cvWaitMutex(&queue->notify, &queue->lock, 15))
        {
            i32 value = queue->buffer[queue->beg];

            queue->beg = (queue->beg + 1) % QueueSize;
            --queue->len;

            fprintf(stdout, "Consumed: %d\n", value);
            fflush(stdout);
        }
        api.mutexRelease(&queue->lock);
    }
}

enum
{
    Consumer = 0,
    Producer = 1,
    Count,
};

int
main(void)
{
    Queue queue = {0};
    api.mutexInit(&queue.lock);

    Thread threads[Count] = {[Producer] = api.threadCreate(&(ThreadParms){
                                 .proc = producerProc,
                                 .args = &queue,
                                 .debug_name = "Producer thread",
                             }),
                             [Consumer] = api.threadCreate(&(ThreadParms){
                                 .proc = consumerProc,
                                 .args = &queue,
                                 .debug_name = "Consumer thread",
                             })};

    api.threadWaitAll(threads, Count, TIMEOUT_INFINITE);

    return 0;
}

#pragma warning(pop)
