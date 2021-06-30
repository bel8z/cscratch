#pragma once

#include "common.h"

#if !defined(CF_THREADING_DEBUG)
#    define CF_THREADING_DEBUG CF_DEBUG
#endif

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

/// Lightweight syncronization primitive which offers exclusive access to a resource.
/// This mutex cannot be acquired recursively.
typedef struct Mutex
{
    U8 data[sizeof(void *)];
#if CF_THREADING_DEBUG
    U32 internal;
#endif
} Mutex;

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

/// Condition variables are synchronization primitives that enable threads to wait until a
/// particular condition occurs.
typedef struct ConditionVariable
{
    U8 data[sizeof(void *)];
} ConditionVariable;

// clang-format off
/// Macro to generate the threading API struct/functions
/// X(name, ReturnType, ...) where ... is the argument list
#define CF_THREADING_API(X)                                                           \
    /*** Misc ***/                                                                 \
    X(sleep, void, Time timeout)                                                 \
    /*** Thread ***/                                                               \
    X(threadCreate,    Thread, ThreadParms *parms)                                 \
    X(threadDestroy,   void,   Thread thread)                                      \
    X(threadIsRunning, bool,   Thread thread)                                      \
    X(threadWait,      bool,   Thread thread, Time timeout)                      \
    X(threadWaitAll,   bool,   Thread *threads, Usize num_threads, Time timeout) \
    /*** Mutex ***/                                                                \
    X(mutexInit,       void, Mutex *mutex)                                         \
    X(mutexShutdown,   void, Mutex *mutex)                                         \
    X(mutexTryAcquire, bool, Mutex *mutex)                                         \
    X(mutexAcquire,    void, Mutex *mutex)                                         \
    X(mutexRelease,    void, Mutex *mutex)                                         \
    /*** RwLock ***/                                                               \
    X(rwInit,          void, RwLock *lock)                                         \
    X(rwShutdown,      void, RwLock *lock)                                         \
    X(rwTryLockReader, bool, RwLock *lock)                                         \
    X(rwTryLockWriter, bool, RwLock *lock)                                         \
    X(rwLockReader,    void, RwLock *lock)                                         \
    X(rwLockWriter,    void, RwLock *lock)                                         \
    X(rwUnlockReader,  void, RwLock *lock)                                         \
    X(rwUnlockWriter,  void, RwLock *lock)                                         \
    /*** ConditionVariable ***/                                                    \
    X(cvInit,       void, ConditionVariable *cv)                                   \
    X(cvShutdown,   void, ConditionVariable *cv)                                   \
    X(cvWaitMutex,  bool, ConditionVariable *cv, Mutex *mutex, Time timeout)     \
    X(cvWaitRwLock, bool, ConditionVariable *cv, RwLock *lock, Time timeout)     \
    X(cvSignalOne,  void, ConditionVariable *cv)                                   \
    X(cvSignalAll,  void, ConditionVariable *cv)
// clang-format on

/// Threading API
typedef struct cfThreading
{
#define ENTRY_FN_(name, ReturnType, ...) ReturnType (*name)(__VA_ARGS__);
    CF_THREADING_API(ENTRY_FN_)
#undef ENTRY_FN_
} cfThreading;

static inline void
threadingCheckApi(cfThreading *api)
{
#define CHECK_ENTRY_FN_(name, ...) CF_ASSERT_NOT_NULL(api->name);
    CF_THREADING_API(CHECK_ENTRY_FN_)
#undef CHECK_ENTRY_FN_
}

/// Wrapper around threadCreate that allows a more convenient syntax for optional
/// parameters
#define threadStart(api, thread_proc, ...) \
    api->threadCreate(&(ThreadParms){.proc = thread_proc, __VA_ARGS__})
