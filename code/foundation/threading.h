#ifndef FOUNDATION_THREADING_H

#include "common.h"

#if !defined(THREADING_DEBUG)
#define THREADING_DEBUG 1
#endif

#define TIMEOUT_INFINITE (u32)(-1)

/// Thread handle
typedef struct Thread
{
    uptr handle;
} Thread;

#define THREAD_PROC(name) void name(void *args)

typedef THREAD_PROC((*ThreadProc));

typedef struct ThreadParms
{
    ThreadProc proc;
    void *args;
    char const *debug_name;
    usize stack_size;
} ThreadParms;

/// Lightweight syncronization primitive which offers exclusive access to a resource.
/// This mutex cannot be acquired recursively.
typedef struct Mutex
{
    u8 data[sizeof(void *)];
#if THREADING_DEBUG
    u32 internal;
#endif
} Mutex;

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

/// Condition variables are synchronization primitives that enable threads to wait until a
/// particular condition occurs.
typedef struct ConditionVariable
{
    u8 data[sizeof(void *)];
} ConditionVariable;

/// Threading API
typedef struct Threading
{
    void (*sleep)(u32 timeout_ms);

    /// Create and start a new thread.
    Thread (*threadCreate)(ThreadParms *parms);
    /// Destroys the given thread handle. A running thread is not terminated by this call, but using
    /// the handle again is undefined behavior.
    void (*threadDestroy)(Thread thread);
    /// Check if the given thread is running
    bool (*threadIsRunning)(Thread thread);
    /// Wait the thread completion for the given amount of time; returns true if the thread
    /// terminates before the timeout occurs, false otherwise.
    bool (*threadWait)(Thread thread, u32 timeout_ms);
    /// Wait the completion of the given threads for the given amount of time; returns true if all
    /// the threads terminate before the timeout occurs, false otherwise.
    bool (*threadWaitAll)(Thread *threads, usize num_threads, u32 timeout_ms);

    void (*mutexInit)(Mutex *mutex);
    void (*mutexShutdown)(Mutex *mutex);
    bool (*mutexTryAcquire)(Mutex *mutex);
    void (*mutexAcquire)(Mutex *mutex);
    void (*mutexRelease)(Mutex *mutex);

    void (*rwInit)(RwLock *lock);
    void (*rwShutdown)(RwLock *lock);
    bool (*rwTryLockReader)(RwLock *lock);
    bool (*rwTryLockWriter)(RwLock *lock);
    void (*rwLockReader)(RwLock *lock);
    void (*rwLockWriter)(RwLock *lock);
    void (*rwUnlockReader)(RwLock *lock);
    void (*rwUnlockWriter)(RwLock *lock);

    void (*cvInit)(ConditionVariable *cv);
    void (*cvShutdown)(ConditionVariable *cv);
    bool (*cvWaitMutex)(ConditionVariable *cv, Mutex *mutex, u32 timeout_ms);
    bool (*cvWaitRwLock)(ConditionVariable *cv, RwLock *lock, u32 timeout_ms);
    void (*cvSignalOne)(ConditionVariable *cv);
    void (*cvSignalAll)(ConditionVariable *cv);

} Threading;

#define FOUNDATION_THREADING_H
#endif
