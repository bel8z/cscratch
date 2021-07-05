#pragma once

#include "common.h"

#if !defined(CF_THREADING_DEBUG)
#    define CF_THREADING_DEBUG CF_DEBUG
#endif

//------------------------------------------------------------------------------
// Threading API
//------------------------------------------------------------------------------

void threadSleep(Time timeout);

//------------------------------------------------------------------------------

/// Thread handle
typedef struct Thread
{
    Uptr handle;
} Thread;

/// Macro to generate a thread procedure signature
#define THREAD_PROC(name) void name(void *args)

/// Pointer to thread procedure
typedef THREAD_PROC((*ThreadProc));

/// Thread creation parameters
typedef struct ThreadParms
{
    ThreadProc proc;
    void *args;
    char const *debug_name;
    Usize stack_size;
} ThreadParms;

Thread threadCreate(ThreadParms *parms);
void threadDestroy(Thread thread);
bool threadIsRunning(Thread thread);
bool threadWait(Thread thread, Time timeout);
bool threadWaitAll(Thread *threads, Usize num_threads, Time timeout);

/// Wrapper around threadCreate that allows a more convenient syntax for optional
/// parameters
#define threadStart(thread_proc, ...) threadCreate(&(ThreadParms){.proc = thread_proc, __VA_ARGS__})

//------------------------------------------------------------------------------

/// Lightweight syncronization primitive which offers exclusive access to a resource.
/// This mutex cannot be acquired recursively.
typedef struct Mutex
{
    U8 data[sizeof(void *)];
#if CF_THREADING_DEBUG
    U32 internal;
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
    U8 data[sizeof(void *)];
#if CF_THREADING_DEBUG
    U32 reserved0;
    U32 reserved1;
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
    U8 data[sizeof(void *)];
} ConditionVariable;

void cvInit(ConditionVariable *cv);
void cvShutdown(ConditionVariable *cv);
bool cvWaitMutex(ConditionVariable *cv, Mutex *mutex, Time timeout);
bool cvWaitRwLock(ConditionVariable *cv, RwLock *lock, Time timeout);
void cvSignalOne(ConditionVariable *cv);
void cvSignalAll(ConditionVariable *cv);

//------------------------------------------------------------------------------
