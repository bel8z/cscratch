#include "win32.h"

#include "foundation/common.h"

#include <stdio.h>
#include <stdlib.h>

//------------------------------------------------------------------------------

#define TIMEOUT_INFINITE (u32)(-1)

/// Thread handle
typedef struct Thread
{
    uptr handle;
} Thread;

typedef void (*ThreadProc)(void *data);

typedef struct ThreadParms
{
    ThreadProc proc;
    void *args;
    usize stack_size;
    bool suspended;
} ThreadParms;

Thread threadCreate(ThreadParms *parms);
void threadDestroy(Thread thread);
void threadStart(Thread thread);
bool threadIsRunning(Thread thread);
bool threadWait(Thread thread, u32 timeout_ms);
bool threadWaitAll(Thread *threads, usize num_threads, u32 timeout_ms);

//------------------------------------------------------------------------------

// TODO (Matteo): Maybe provide shared access for multiple readers?

/// Lightweight syncronization primitive which offers exclusive access to a resource
/// This mutex cannot be acquired recursively
typedef struct Mutex
{
    u8 data[sizeof(void *)];
    // DEBUG
    u32 internal;
} Mutex;

void mutexInit(Mutex *mutex);
void mutexShutdown(Mutex *mutex);
void mutexAcquire(Mutex *mutex);
void mutexRelease(Mutex *mutex);

//------------------------------------------------------------------------------

/// Condition variables are synchronization primitives that enable threads to wait until a
/// particular condition occurs.
typedef struct ConditionVariable
{
    u8 data[sizeof(void *)];
} ConditionVariable;

void cvInit(ConditionVariable *cv);
void cvShutdown(ConditionVariable *cv);
bool cvWait(ConditionVariable *cv, Mutex *mutex, u32 timeout_ms);
void cvSignalOne(ConditionVariable *cv);
void cvSignalAll(ConditionVariable *cv);

//------------------------------------------------------------------------------

CF_STATIC_ASSERT(sizeof(Thread) == sizeof(HANDLE), "Thread and HANDLE size must be equal");
CF_STATIC_ASSERT(alignof(Thread) == alignof(HANDLE), "Thread must be aligned as HANDLE");
CF_STATIC_ASSERT(sizeof(((Mutex *)0)->data) == sizeof(SRWLOCK), "Invalid Mutex internal size");
CF_STATIC_ASSERT(sizeof(((ConditionVariable *)0)->data) == sizeof(CONDITION_VARIABLE),
                 "Invalid ConditionVariable internal size");

u32 WINAPI
win32ThreadProc(void *data)
{
    ThreadParms *parms = data;

    ThreadProc proc = parms->proc;
    void *args = parms->args;

    HeapFree(GetProcessHeap(), 0, parms);

    proc(args);

    _endthreadex(0);

    return 0;
}

Thread
threadCreate(ThreadParms *parms)
{
    Thread thread = {0};
    ThreadParms *parms_copy = HeapAlloc(GetProcessHeap(), 0, sizeof(*parms_copy));

    if (parms_copy)
    {
        *parms_copy = *parms;

        CF_ASSERT(parms_copy->stack_size <= U32_MAX, "Required stack size is too large");

        u32 flags = parms_copy->suspended ? CREATE_SUSPENDED : 0;
        thread.handle = _beginthreadex(NULL, (u32)parms_copy->stack_size, win32ThreadProc,
                                       parms_copy, flags, NULL);
    }

    return thread;
}

void
threadDestroy(Thread thread)
{
    if (thread.handle)
    {
        CloseHandle((HANDLE)thread.handle);
    }
}

void
threadStart(Thread thread)
{
    if (thread.handle)
    {
        ResumeThread((HANDLE)thread.handle);
    }
}

bool
threadIsRunning(Thread thread)
{
    DWORD code = 0;
    return (GetExitCodeThread((HANDLE)thread.handle, &code) && code == STILL_ACTIVE);
}

bool
threadWait(Thread thread, u32 timeout_ms)
{
    return (thread.handle &&
            WAIT_OBJECT_0 == WaitForSingleObject((HANDLE)thread.handle, timeout_ms));
}

bool
threadWaitAll(Thread *threads, usize num_threads, u32 timeout_ms)
{
    CF_ASSERT(num_threads <= U32_MAX, "Given number of threads is too large");
    DWORD count = (DWORD)num_threads;
    DWORD code = WaitForMultipleObjects(count, (HANDLE *)threads, true, timeout_ms);
    return (code < WAIT_OBJECT_0 + count);
}

//------------------------------------------------------------------------------

void
mutexInit(Mutex *mutex)
{
    CF_ASSERT_NOT_NULL(mutex);
    InitializeSRWLock((SRWLOCK *)(mutex->data));
}

void
mutexShutdown(Mutex *mutex)
{
    CF_ASSERT_NOT_NULL(mutex);
    CF_ASSERT(mutex->internal == 0, "Shutting down an acquired mutex");
}

void
mutexAcquire(Mutex *mutex)
{
    CF_ASSERT_NOT_NULL(mutex);

    SRWLOCK *lock = (SRWLOCK *)(mutex->data);

    // NOTE (Matteo): Naive check for recursive access
    if (!TryAcquireSRWLockExclusive(lock))
    {
        CF_ASSERT(mutex->internal != GetCurrentThreadId(), "Attempted to lock Mutex recursively");
        AcquireSRWLockExclusive(lock);
    }

    mutex->internal = GetCurrentThreadId();
}

void
mutexRelease(Mutex *mutex)
{
    CF_ASSERT_NOT_NULL(mutex);
    mutex->internal = 0;
    ReleaseSRWLockExclusive((SRWLOCK *)(mutex->data));
}

//------------------------------------------------------------------------------

void
cvInit(ConditionVariable *cv)
{
    CF_ASSERT_NOT_NULL(cv);
    InitializeConditionVariable((CONDITION_VARIABLE *)(cv->data));
}

void
cvShutdown(ConditionVariable *cv)
{
    CF_ASSERT_NOT_NULL(cv);
}

bool
cvWait(ConditionVariable *cv, Mutex *mutex, u32 timeout_ms)
{
    CF_ASSERT_NOT_NULL(cv);
    CF_ASSERT_NOT_NULL(mutex);
    return SleepConditionVariableSRW((CONDITION_VARIABLE *)(cv->data), (SRWLOCK *)(mutex->data),
                                     timeout_ms, 0);
}

void
cvSignalOne(ConditionVariable *cv)
{
    CF_ASSERT_NOT_NULL(cv);
    WakeConditionVariable((CONDITION_VARIABLE *)(cv->data));
}

void
cvSignalAll(ConditionVariable *cv)
{
    CF_ASSERT_NOT_NULL(cv);
    WakeAllConditionVariable((CONDITION_VARIABLE *)(cv->data));
}

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

void
myThreadProc(void *parm)
{
    Queue *queue = parm;

    CF_UNUSED(queue);

    for (i32 i = 10; i >= 0; --i)
    {
        fprintf(stdout, "\r%d", i);
        fflush(stdout);
        Sleep(1000);
    }
}

void
producerProc(void *parm)
{
    Queue *queue = parm;

    while (true)
    {
        Sleep(1000);
        mutexAcquire(&queue->lock);

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

        cvSignalOne(&queue->notify);
        mutexRelease(&queue->lock);
    }
}

void
consumerProc(void *parm)
{
    Queue *queue = parm;

    while (true)
    {
        mutexAcquire(&queue->lock);
        if (cvWait(&queue->notify, &queue->lock, 15))
        {
            i32 value = queue->buffer[queue->beg];

            queue->beg = (queue->beg + 1) % QueueSize;
            --queue->len;

            fprintf(stdout, "Consumed: %d\n", value);
            fflush(stdout);
        }
        mutexRelease(&queue->lock);
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
    mutexInit(&queue.lock);

    Thread threads[Count] = {[Producer] = threadCreate(&(ThreadParms){
                                 .proc = producerProc,
                                 .args = &queue,
                                 .suspended = true,
                             }),
                             [Consumer] = threadCreate(&(ThreadParms){
                                 .proc = consumerProc,
                                 .args = &queue,
                                 .suspended = true,
                             })};

    threadStart(threads[Consumer]);
    threadStart(threads[Producer]);

    threadWaitAll(threads, Count, TIMEOUT_INFINITE);

    return 0;
}

#pragma warning(pop)
