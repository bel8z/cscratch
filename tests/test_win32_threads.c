#include "win32.h"

#include "foundation/common.h"

#include <stdio.h>
#include <stdlib.h>

//------------------------------------------------------------------------------

#define THREADING_DEBUG 1

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
    char const *debug_name;
    usize stack_size;
} ThreadParms;

/// Create a thread. The returned thread is not running, and must be explicitly started.
Thread threadCreate(ThreadParms *parms);
/// Create a running thread.
Thread threadSpawn(ThreadParms *parms);
/// Destroys the given thread handle. A running thread is not terminated by this call, but using the
/// handle again is undefined behavior.
void threadDestroy(Thread thread);
/// Start a thread created with threadCreate
void threadStart(Thread thread);
/// Check if the given thread is running
bool threadIsRunning(Thread thread);
/// Wait the thread completion for the given amount of time; returns true if the thread terminates
/// before the timeout occurs, false otherwise.
bool threadWait(Thread thread, u32 timeout_ms);
/// Wait the completion of the given threads for the given amount of time; returns true if all the
/// threads terminate before the timeout occurs, false otherwise.
bool threadWaitAll(Thread *threads, usize num_threads, u32 timeout_ms);

//------------------------------------------------------------------------------

/// Lightweight syncronization primitive which offers exclusive access to a resource.
/// This mutex cannot be acquired recursively.
typedef struct Mutex
{
    u8 data[sizeof(void *)];
#if THREADING_DEBUG
    u32 internal;
#endif
} Mutex;

void mutexInit(Mutex *mutex);
void mutexShutdown(Mutex *mutex);

bool mutexTryAcquire(Mutex *mutex);
void mutexAcquire(Mutex *mutex);
void mutexRelease(Mutex *mutex);

//------------------------------------------------------------------------------

/// Lightweight syncronization primitive which offers shared access to readers and
/// exclusive access to writers of a resource.
/// The lock cannot be taken recursively, neither by readers nor writers.
typedef struct RwLock
{
    u8 data[sizeof(void *)];
#if THREADING_DEBUG
    u32 reserved0;
    u32 reserved1;
#endif
} RwLock;

void rwInit(RwLock *lock);
void rwShutdown(RwLock *lock);

bool rwTryLockReader(RwLock *lock);
bool rwTryLockWriter(RwLock *lock);
void rwLockReader(RwLock *lock);
void rwLockWriter(RwLock *lock);
void rwUnlockReader(RwLock *lock);
void rwUnlockWriter(RwLock *lock);

//------------------------------------------------------------------------------

/// Condition variables are synchronization primitives that enable threads to wait until a
/// particular condition occurs.
typedef struct ConditionVariable
{
    u8 data[sizeof(void *)];
} ConditionVariable;

void cvInit(ConditionVariable *cv);
void cvShutdown(ConditionVariable *cv);

bool cvWaitMutex(ConditionVariable *cv, Mutex *mutex, u32 timeout_ms);
bool cvWaitRwLock(ConditionVariable *cv, RwLock *lock, u32 timeout_ms);
void cvSignalOne(ConditionVariable *cv);
void cvSignalAll(ConditionVariable *cv);

//------------------------------------------------------------------------------

CF_STATIC_ASSERT(sizeof(Thread) == sizeof(HANDLE), "Thread and HANDLE size must be equal");
CF_STATIC_ASSERT(alignof(Thread) == alignof(HANDLE), "Thread must be aligned as HANDLE");

CF_STATIC_ASSERT(sizeof(((Mutex *)0)->data) == sizeof(SRWLOCK), "Invalid Mutex internal size");

CF_STATIC_ASSERT(sizeof(((RwLock *)0)->data) == sizeof(SRWLOCK), "Invalid RwLock internal size");

CF_STATIC_ASSERT(sizeof(((ConditionVariable *)0)->data) == sizeof(CONDITION_VARIABLE),
                 "Invalid ConditionVariable internal size");

static u32 WINAPI
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

static Thread
internalThreadCreate(ThreadParms *parms, bool suspended)
{
    Thread thread = {0};

    // TODO (Matteo): Use dedicated, lighter, data structure?
    ThreadParms *parms_copy = HeapAlloc(GetProcessHeap(), 0, sizeof(*parms_copy));

    if (parms_copy)
    {
        *parms_copy = *parms;

        CF_ASSERT(parms_copy->stack_size <= U32_MAX, "Required stack size is too large");

        u32 flags = suspended ? CREATE_SUSPENDED : 0;
        thread.handle = _beginthreadex(NULL, (u32)parms_copy->stack_size, win32ThreadProc,
                                       parms_copy, flags, NULL);
    }

    if (thread.handle && parms->debug_name)
    {
        WCHAR buffer[1024];
        u32 size = MultiByteToWideChar(CP_UTF8, 0, parms->debug_name, -1, buffer, 1024);
        CF_ASSERT(size > 0 || size < 1024, "Thread debug name is too long");
        SetThreadDescription((HANDLE)thread.handle, buffer);
    }

    return thread;
}

Thread
threadCreate(ThreadParms *parms)
{
    return internalThreadCreate(parms, true);
}

Thread
threadSpawn(ThreadParms *parms)
{
    return internalThreadCreate(parms, false);
}

void
threadDestroy(Thread thread)
{
    if (thread.handle) CloseHandle((HANDLE)thread.handle);
}

void
threadStart(Thread thread)
{
    if (thread.handle) ResumeThread((HANDLE)thread.handle);
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

// Internal implementation of exclusive locking, shared by Mutex and RwLock

#if THREADING_DEBUG

static bool
internalTryLockExc(SRWLOCK *lock, u32 *owner_id)
{
    CF_ASSERT_NOT_NULL(lock);
    CF_ASSERT_NOT_NULL(owner_id);

    // NOTE (Matteo): Naive check for recursive access
    if (!TryAcquireSRWLockExclusive(lock))
    {
        CF_ASSERT(*owner_id != GetCurrentThreadId(), "Attempted to lock recursively");
        return false;
    }

    *owner_id = GetCurrentThreadId();
    return true;
}

static void
internalLockExc(SRWLOCK *lock, u32 *owner_id)
{
    CF_ASSERT_NOT_NULL(lock);
    CF_ASSERT_NOT_NULL(owner_id);

    // NOTE (Matteo): Naive check for recursive access
    if (!TryAcquireSRWLockExclusive(lock))
    {
        CF_ASSERT(*owner_id != GetCurrentThreadId(), "Attempted to lock recursively");
        AcquireSRWLockExclusive(lock);
    }

    *owner_id = GetCurrentThreadId();
}

static void
internalUnlockExc(SRWLOCK *lock, u32 *owner_id)
{
    CF_ASSERT_NOT_NULL(lock);
    CF_ASSERT_NOT_NULL(owner_id);
    *owner_id = 0;
    ReleaseSRWLockExclusive(lock);
}

#endif

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
#if THREADING_DEBUG
    CF_ASSERT(mutex->internal == 0, "Shutting down an acquired mutex");
#endif
}

bool
mutexTryAcquire(Mutex *mutex)
{
    CF_ASSERT_NOT_NULL(mutex);
#if THREADING_DEBUG
    return internalTryLockExc((SRWLOCK *)(mutex->data), &mutex->internal);
#else
    return TryAcquireSRWLockExclusive((SRWLOCK *)(mutex->data));
#endif
}

void
mutexAcquire(Mutex *mutex)
{
    CF_ASSERT_NOT_NULL(mutex);
#if THREADING_DEBUG
    internalLockExc((SRWLOCK *)(mutex->data), &mutex->internal);
#else
    AcquireSRWLockExclusive((SRWLOCK *)(mutex->data));
#endif
}

void
mutexRelease(Mutex *mutex)
{
    CF_ASSERT_NOT_NULL(mutex);
#if THREADING_DEBUG
    internalUnlockExc((SRWLOCK *)(mutex->data), &mutex->internal);
#else
    ReleaseSRWLockExclusive((SRWLOCK *)(mutex->data));
#endif
}

//------------------------------------------------------------------------------
void
rwInit(RwLock *lock)
{
    CF_ASSERT_NOT_NULL(lock);
    InitializeSRWLock((SRWLOCK *)(lock->data));
}

void
rwShutdown(RwLock *lock)
{
    CF_ASSERT_NOT_NULL(lock);
#if THREADING_DEBUG
    CF_ASSERT(lock->reserved0 == 0, "Shutting down an RwLock acquired for writing");
    CF_ASSERT(lock->reserved1 == 0, "Shutting down an RwLock acquired for reading");
#endif
}

bool
rwTryLockReader(RwLock *lock)
{
    CF_ASSERT_NOT_NULL(lock);
    if (TryAcquireSRWLockShared((SRWLOCK *)(lock->data)))
    {
#if THREADING_DEBUG
        ++lock->reserved1;
#endif
        return true;
    }
    return false;
}

void
rwLockReader(RwLock *lock)
{
    CF_ASSERT_NOT_NULL(lock);
    AcquireSRWLockShared((SRWLOCK *)(lock->data));
#if THREADING_DEBUG
    ++lock->reserved1;
#endif
}

void
rwUnlockReader(RwLock *lock)
{
    CF_ASSERT_NOT_NULL(lock);
#if THREADING_DEBUG
    --lock->reserved1;
#endif
    ReleaseSRWLockShared((SRWLOCK *)(lock->data));
}

bool
rwTryLockWriter(RwLock *lock)
{
    CF_ASSERT_NOT_NULL(lock);
#if THREADING_DEBUG
    return internalTryLockExc((SRWLOCK *)(lock->data), &lock->reserved0);
#else
    return TryAcquireSRWLockExclusive((SRWLOCK *)(lock->data));
#endif
}

void
rwLockWriter(RwLock *lock)
{
    CF_ASSERT_NOT_NULL(lock);
#if THREADING_DEBUG
    internalLockExc((SRWLOCK *)(lock->data), &lock->reserved0);
#else
    AcquireSRWLockExclusive((SRWLOCK *)(lock->data));
#endif
}

void
rwUnlockWriter(RwLock *lock)
{
    CF_ASSERT_NOT_NULL(lock);
#if THREADING_DEBUG
    internalUnlockExc((SRWLOCK *)(lock->data), &lock->reserved0);
#else
    ReleaseSRWLockExclusive((SRWLOCK *)(lock->data));
#endif
}

//------------------------------------------------------------------------------

static inline bool
cvWait(ConditionVariable *cv, SRWLOCK *lock, u32 timeout_ms)
{
    CF_ASSERT_NOT_NULL(cv);
    return SleepConditionVariableSRW((CONDITION_VARIABLE *)(cv->data), lock, timeout_ms, 0);
}

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
cvWaitMutex(ConditionVariable *cv, Mutex *mutex, u32 timeout_ms)
{
    CF_ASSERT_NOT_NULL(mutex);
#if THREADING_DEBUG
    CF_ASSERT(mutex->internal != 0, "Attemped wait on unlocked Mutex");
#endif
    return cvWait(cv, (SRWLOCK *)(mutex->data), timeout_ms);
}

bool
cvWaitRwLock(ConditionVariable *cv, RwLock *lock, u32 timeout_ms)
{
    CF_ASSERT_NOT_NULL(lock);
#if THREADING_DEBUG
    CF_ASSERT(lock->reserved0 != 0 || lock->reserved1 != 0, "Attemped wait on unlocked RwLock");
#endif
    return cvWait(cv, (SRWLOCK *)(lock->data), timeout_ms);
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
        if (cvWaitMutex(&queue->notify, &queue->lock, 15))
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
                                 .debug_name = "Producer thread",
                             }),
                             [Consumer] = threadCreate(&(ThreadParms){
                                 .proc = consumerProc,
                                 .args = &queue,
                                 .debug_name = "Consumer thread",
                             })};

    threadStart(threads[Consumer]);
    threadStart(threads[Producer]);

    threadWaitAll(threads, Count, TIMEOUT_INFINITE);

    return 0;
}

#pragma warning(pop)
