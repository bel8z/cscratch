#include "foundation/common.h"

#include <stdio.h>

#pragma warning(push)
#pragma warning(disable : 5105)

#define NOMINMAX 1
#define VC_EXTRALEAN 1
#define WIN32_LEAN_AND_MEAN 1
#include <windows.h>
// Must be included AFTER <windows.h>
#include <commdlg.h>
#include <process.h>
#include <shellapi.h>

#pragma warning(pop)

//------------------------------------------------------------------------------

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
bool threadWait(Thread thread, u32 ms);
bool threadWaitAll(Thread *threads, usize num_threads, u32 ms);

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
void cvWait(ConditionVariable *cv, Mutex *mutex, u32 ms);
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
threadWait(Thread thread, u32 ms)
{
    return (thread.handle && WAIT_OBJECT_0 == WaitForSingleObject((HANDLE)thread.handle, ms));
}

bool
threadWaitAll(Thread *threads, usize num_threads, u32 ms)
{
    CF_ASSERT(num_threads <= U32_MAX, "Given number of threads is too large");
    DWORD count = (DWORD)num_threads;
    DWORD code = WaitForMultipleObjects(count, (HANDLE *)threads, true, ms);
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
cvWait(ConditionVariable *cv, Mutex *mutex, u32 ms)
{
    CF_ASSERT_NOT_NULL(cv);
    CF_ASSERT_NOT_NULL(mutex);
    SleepConditionVariableSRW((CONDITION_VARIABLE *)(cv->data), (SRWLOCK *)(mutex->data), ms, 0);
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
    u32 buffer[QueueSize];
    u32 pos;
    Mutex lock;
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

int
main(void)
{
    Queue queue = {0};
    mutexInit(&queue.lock);

    Thread thread =
        threadCreate(&(ThreadParms){.proc = myThreadProc, .suspended = true, .args = &queue});

    printf("\nThread created\n");

    threadStart(thread);

    printf("\nThread started\n");

    while (threadIsRunning(thread))
    {
        Sleep(15);
    }

    printf("\nThread terminated\n");

    return 0;
}

#pragma warning(pop)
