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

// clang-format off
/// Macro to generate the threading API struct/functions
/// X(name, ReturnType, ...) where ... is the argument list
#define THREADING_API(X)                                                           \
    /*** Misc ***/                                                                 \
    X(sleep, void, u32 timeout_ms)                                                 \
    /*** Thread ***/                                                               \
    X(threadCreate,    Thread, ThreadParms *parms)                                 \
    X(threadDestroy,   void,   Thread thread)                                      \
    X(threadIsRunning, bool,   Thread thread)                                      \
    X(threadWait,      bool,   Thread thread, u32 timeout_ms)                      \
    X(threadWaitAll,   bool,   Thread *threads, usize num_threads, u32 timeout_ms) \
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
    X(cvWaitMutex,  bool, ConditionVariable *cv, Mutex *mutex, u32 timeout_ms)     \
    X(cvWaitRwLock, bool, ConditionVariable *cv, RwLock *lock, u32 timeout_ms)     \
    X(cvSignalOne,  void, ConditionVariable *cv)                                   \
    X(cvSignalAll,  void, ConditionVariable *cv)
// clang-format on

/// Threading API
typedef struct Threading
{
#define ENTRY_FN(name, ReturnType, ...) ReturnType (*name)(__VA_ARGS__);
    THREADING_API(ENTRY_FN)
#undef ENTRY_FN
} Threading;

static inline void
threadingCheckApi(Threading *api)
{
#define CHECK_ENTRY_FN(name, ...) CF_ASSERT_NOT_NULL(api->name);
    THREADING_API(CHECK_ENTRY_FN)
#undef CHECK_ENTRY_FN
}

#define FOUNDATION_THREADING_H
#endif
