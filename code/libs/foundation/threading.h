#pragma once

//------------------------------------------------------------------------------

/// Foundation threading support
/// This is an API header and as such the only included header must be "core.h"

// TODO (Matteo):
// * Different "namespace" prefix for threading API?
// * API for querying/setting thread affinity?

//------------------------------------------------------------------------------

#include "atom.h"
#include "core.h"

#if !defined(CF_THREADING_DEBUG)
#    define CF_THREADING_DEBUG CF_DEBUG
#endif

//---------------------//
//   Basic utilities   //
//---------------------//

/// Number of logical processors available on the machine
Usize cfNumCores(void);

/// Suspends the execution of the current thread until the given duration elapses
void cfSleep(Duration duration);

/// Causes the calling thread to yield execution to another thread that is ready to run on the
/// current processor. The operating system selects the next thread to be executed.
void cfYield(void);

/// Retrieves the thread identifier of the calling thread.
U32 cfCurrentThreadId(void);

//------------//
//   Thread   //
//------------//

/// Thread handle
typedef struct CfThread
{
    Uptr handle;
} CfThread;

/// Macro to generate a thread procedure signature
#define CF_THREAD_FN(name) void name(void *args)

/// Pointer to thread procedure
typedef CF_THREAD_FN((*CfThreadFn));

/// Thread creation parameters
typedef struct CfThreadParms
{
    CfThreadFn fn;
    void *args;
    Char8 const *debug_name;
    Usize stack_size;
} CfThreadParms;

CF_API CfThread cfThreadCreate(CfThreadParms *parms);
CF_API void cfThreadDestroy(CfThread thread);
CF_API bool cfThreadIsRunning(CfThread thread);
CF_API bool cfThreadWait(CfThread thread, Duration duration);
CF_API bool cfThreadWaitAll(CfThread *threads, Usize num_threads, Duration duration);
CF_API Usize cfThreadWaitAny(CfThread *threads, Usize num_threads, Duration duration);

/// Wrapper around threadCreate that allows a more convenient syntax for optional
/// parameters
#define cfThreadStart(thread_fn, ...) cfThreadCreate(&(CfThreadParms){.fn = thread_fn, __VA_ARGS__})

//-----------//
//   Mutex   //
//-----------//

/// Lightweight syncronization primitive which offers exclusive access to a resource.
/// This mutex cannot be acquired recursively.
typedef struct CfMutex
{
    U8 data[CF_PTR_SIZE];
#if CF_THREADING_DEBUG
    U32 internal;
#endif
} CfMutex;

CF_API void cfMutexInit(CfMutex *mutex);
CF_API void cfMutexShutdown(CfMutex *mutex);
CF_API bool cfMutexTryAcquire(CfMutex *mutex);
CF_API void cfMutexAcquire(CfMutex *mutex);
CF_API void cfMutexRelease(CfMutex *mutex);

//------------------------//
//   Reader/writer lock   //
//------------------------//

/// Lightweight syncronization primitive which offers shared access to readers and
/// exclusive access to writers of a resource.
/// The lock cannot be taken recursively, neither by readers nor writers.
typedef struct CfRwLock
{
    U8 data[CF_PTR_SIZE];
#if CF_THREADING_DEBUG
    U32 reserved0;
    U32 reserved1;
#endif
} CfRwLock;

CF_API void cfRwInit(CfRwLock *lock);
CF_API void cfRwShutdown(CfRwLock *lock);
CF_API bool cfRwTryLockReader(CfRwLock *lock);
CF_API bool cfRwTryLockWriter(CfRwLock *lock);
CF_API void cfRwLockReader(CfRwLock *lock);
CF_API void cfRwLockWriter(CfRwLock *lock);
CF_API void cfRwUnlockReader(CfRwLock *lock);
CF_API void cfRwUnlockWriter(CfRwLock *lock);

//-------------------------//
//   Condition variables   //
//-------------------------//

/// Condition variables are synchronization primitives that enable threads to wait until a
/// particular condition occurs.
typedef struct CfConditionVariable
{
    U8 data[CF_PTR_SIZE];
} CfConditionVariable;

CF_API void cfCvInit(CfConditionVariable *cv);
CF_API void cfCvShutdown(CfConditionVariable *cv);
CF_API bool cfCvWaitMutex(CfConditionVariable *cv, CfMutex *mutex, Duration duration);
CF_API bool cfCvWaitRwLock(CfConditionVariable *cv, CfRwLock *lock, Duration duration);
CF_API void cfCvSignalOne(CfConditionVariable *cv);
CF_API void cfCvSignalAll(CfConditionVariable *cv);

//----------------//
//   Semaphores   //
//----------------//

typedef void *CfSemaphoreHandle;

typedef struct CfSemaphore
{
    CfSemaphoreHandle handle;
    AtomIsize count;
} CfSemaphore;

CF_API void cfSemaInit(CfSemaphore *sema, Usize init_count);
CF_API bool cfSemaTryWait(CfSemaphore *sema);
CF_API void cfSemaWait(CfSemaphore *sema);
CF_API void cfSemaSignalOne(CfSemaphore *sema);
CF_API void cfSemaSignal(CfSemaphore *sema, Usize count);

//------------------------------------------------------------------------------
