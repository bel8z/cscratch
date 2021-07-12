#pragma once

//------------------------------------------------------------------------------

/// Foundation threading support
/// This is an API header and as such the only included header must be "core.h"

// TODO (Matteo): Different "namespace" prefix for threading API?

//------------------------------------------------------------------------------

#include "core.h"

#if !defined(CF_THREADING_DEBUG)
#    define CF_THREADING_DEBUG CF_DEBUG
#endif

//---------------------//
//   Basic utilities   //
//---------------------//

void cfSleep(Time timeout);

//------------//
//   Thread   //
//------------//

/// Thread handle
typedef struct CfThread
{
    Uptr handle;
} CfThread;

/// Macro to generate a thread procedure signature
#define CF_THREAD_PROC(name) void name(void *args)

/// Pointer to thread procedure
typedef CF_THREAD_PROC((*CfThreadProc));

/// Thread creation parameters
typedef struct CfThreadParms
{
    CfThreadProc proc;
    void *args;
    char const *debug_name;
    Usize stack_size;
} CfThreadParms;

CfThread cfThreadCreate(CfThreadParms *parms);
void cfThreadDestroy(CfThread thread);
bool cfThreadIsRunning(CfThread thread);
bool cfThreadWait(CfThread thread, Time timeout);
bool cfThreadWaitAll(CfThread *threads, Usize num_threads, Time timeout);

/// Wrapper around threadCreate that allows a more convenient syntax for optional
/// parameters
#define cfThreadStart(thread_proc, ...) \
    cfThreadCreate(&(CfThreadParms){.proc = thread_proc, __VA_ARGS__})

//-----------//
//   Mutex   //
//-----------//

/// Lightweight syncronization primitive which offers exclusive access to a resource.
/// This mutex cannot be acquired recursively.
typedef struct CfMutex
{
    U8 data[sizeof(void *)];
#if CF_THREADING_DEBUG
    U32 internal;
#endif
} CfMutex;

void cfMutexInit(CfMutex *mutex);
void cfMutexShutdown(CfMutex *mutex);
bool cfMutexTryAcquire(CfMutex *mutex);
void cfMutexAcquire(CfMutex *mutex);
void cfMutexRelease(CfMutex *mutex);

//------------------------//
//   Reader/writer lock   //
//------------------------//

/// Lightweight syncronization primitive which offers shared access to readers and
/// exclusive access to writers of a resource.
/// The lock cannot be taken recursively, neither by readers nor writers.
typedef struct CfRwLock
{
    U8 data[sizeof(void *)];
#if CF_THREADING_DEBUG
    U32 reserved0;
    U32 reserved1;
#endif
} CfRwLock;

void cfRwInit(CfRwLock *lock);
void cfRwShutdown(CfRwLock *lock);
bool cfRwTryLockReader(CfRwLock *lock);
bool cfRwTryLockWriter(CfRwLock *lock);
void cfRwLockReader(CfRwLock *lock);
void cfRwLockWriter(CfRwLock *lock);
void cfRwUnlockReader(CfRwLock *lock);
void cfRwUnlockWriter(CfRwLock *lock);

//-------------------------//
//   Condition variables   //
//-------------------------//

/// Condition variables are synchronization primitives that enable threads to wait until a
/// particular condition occurs.
typedef struct CfConditionVariable
{
    U8 data[sizeof(void *)];
} CfConditionVariable;

void cfCvInit(CfConditionVariable *cv);
void cfCvShutdown(CfConditionVariable *cv);
bool cfCvWaitMutex(CfConditionVariable *cv, CfMutex *mutex, Time timeout);
bool cfCvWaitRwLock(CfConditionVariable *cv, CfRwLock *lock, Time timeout);
void cfCvSignalOne(CfConditionVariable *cv);
void cfCvSignalAll(CfConditionVariable *cv);

//------------------------------------------------------------------------------
