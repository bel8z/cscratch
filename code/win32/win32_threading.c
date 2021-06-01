#include "win32.h"

#include "foundation/common.h"

#include "foundation/allocator.h"
#include "foundation/threading.h"

//------------------------------------------------------------------------------

static cfAllocator *g_alloc = NULL;

#define DECLARE_FN(name, ReturnType, ...) static ReturnType win32##name(__VA_ARGS__);
THREADING_API(DECLARE_FN)
#undef DECLARE_FN

//------------------------------------------------------------------------------
// API initialization

void
win32ThreadingInit(Threading *api, cfAllocator *allocator)
{
    CF_ASSERT_NOT_NULL(allocator);

    g_alloc = allocator;

#define ASSIGN_FN(name, ...) api->name = win32##name;
    THREADING_API(ASSIGN_FN)
#undef ASSIGN_FN

    threadingCheckApi(api);
}

//------------------------------------------------------------------------------
// Misc implementation

static void
win32sleep(u32 ms)
{
    Sleep(ms);
}

//------------------------------------------------------------------------------
// Thread implementation

CF_STATIC_ASSERT(sizeof(Thread) == sizeof(HANDLE), "Thread and HANDLE size must be equal");
CF_STATIC_ASSERT(alignof(Thread) == alignof(HANDLE), "Thread must be aligned as HANDLE");

static u32 WINAPI
win32threadProc(void *data)
{
    ThreadParms *parms = data;

    ThreadProc proc = parms->proc;
    void *args = parms->args;

    cfFree(g_alloc, parms, sizeof(*parms));

    proc(args);

    _endthreadex(0);

    return 0;
}

Thread
win32threadCreate(ThreadParms *parms)
{
    Thread thread = {0};

    // TODO (Matteo): Use dedicated, lighter, data structure?
    ThreadParms *parms_copy = cfAlloc(g_alloc, sizeof(*parms_copy));

    if (parms_copy)
    {
        *parms_copy = *parms;

        CF_ASSERT(parms_copy->stack_size <= U32_MAX, "Required stack size is too large");

        thread.handle = _beginthreadex(NULL, (u32)parms_copy->stack_size, win32threadProc,
                                       parms_copy, CREATE_SUSPENDED, NULL);
    }

    if (thread.handle && parms->debug_name)
    {
        WCHAR buffer[1024];
        u32 size = MultiByteToWideChar(CP_UTF8, 0, parms->debug_name, -1, buffer, 1024);
        CF_ASSERT(size > 0 || size < 1024, "Thread debug name is too long");
        SetThreadDescription((HANDLE)thread.handle, buffer);
        ResumeThread((HANDLE)thread.handle);
    }

    return thread;
}

void
win32threadDestroy(Thread thread)
{
    if (thread.handle) CloseHandle((HANDLE)thread.handle);
}

bool
win32threadIsRunning(Thread thread)
{
    DWORD code = 0;
    return (GetExitCodeThread((HANDLE)thread.handle, &code) && code == STILL_ACTIVE);
}

bool
win32threadWait(Thread thread, u32 timeout_ms)
{
    return (thread.handle &&
            WAIT_OBJECT_0 == WaitForSingleObject((HANDLE)thread.handle, timeout_ms));
}

bool
win32threadWaitAll(Thread *threads, usize num_threads, u32 timeout_ms)
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
win32tryLockExc(SRWLOCK *lock, u32 *owner_id)
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
win32lockExc(SRWLOCK *lock, u32 *owner_id)
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
win32unlockExc(SRWLOCK *lock, u32 *owner_id)
{
    CF_ASSERT_NOT_NULL(lock);
    CF_ASSERT_NOT_NULL(owner_id);
    *owner_id = 0;
    ReleaseSRWLockExclusive(lock);
}

#endif

//------------------------------------------------------------------------------
// Mutex implementation

CF_STATIC_ASSERT(sizeof(((Mutex *)0)->data) == sizeof(SRWLOCK), "Invalid Mutex internal size");

void
win32mutexInit(Mutex *mutex)
{
    CF_ASSERT_NOT_NULL(mutex);
    InitializeSRWLock((SRWLOCK *)(mutex->data));
#if THREADING_DEBUG
    mutex->internal = 0;
#endif
}

void
win32mutexShutdown(Mutex *mutex)
{
    CF_ASSERT_NOT_NULL(mutex);
#if THREADING_DEBUG
    CF_ASSERT(mutex->internal == 0, "Shutting down an acquired mutex");
#endif
}

bool
win32mutexTryAcquire(Mutex *mutex)
{
    CF_ASSERT_NOT_NULL(mutex);
#if THREADING_DEBUG
    return win32tryLockExc((SRWLOCK *)(mutex->data), &mutex->internal);
#else
    return TryAcquireSRWLockExclusive((SRWLOCK *)(mutex->data));
#endif
}

void
win32mutexAcquire(Mutex *mutex)
{
    CF_ASSERT_NOT_NULL(mutex);
#if THREADING_DEBUG
    win32lockExc((SRWLOCK *)(mutex->data), &mutex->internal);
#else
    AcquireSRWLockExclusive((SRWLOCK *)(mutex->data));
#endif
}

void
win32mutexRelease(Mutex *mutex)
{
    CF_ASSERT_NOT_NULL(mutex);
#if THREADING_DEBUG
    win32unlockExc((SRWLOCK *)(mutex->data), &mutex->internal);
#else
    ReleaseSRWLockExclusive((SRWLOCK *)(mutex->data));
#endif
}

//------------------------------------------------------------------------------
// RwLock implementation

CF_STATIC_ASSERT(sizeof(((RwLock *)0)->data) == sizeof(SRWLOCK), "Invalid RwLock internal size");

void
win32rwInit(RwLock *lock)
{
    CF_ASSERT_NOT_NULL(lock);
    InitializeSRWLock((SRWLOCK *)(lock->data));
#if THREADING_DEBUG
    lock->reserved0 = 0;
    lock->reserved1 = 0;
#endif
}

void
win32rwShutdown(RwLock *lock)
{
    CF_ASSERT_NOT_NULL(lock);
#if THREADING_DEBUG
    CF_ASSERT(lock->reserved0 == 0, "Shutting down an RwLock acquired for writing");
    CF_ASSERT(lock->reserved1 == 0, "Shutting down an RwLock acquired for reading");
#endif
}

bool
win32rwTryLockReader(RwLock *lock)
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
win32rwLockReader(RwLock *lock)
{
    CF_ASSERT_NOT_NULL(lock);
    AcquireSRWLockShared((SRWLOCK *)(lock->data));
#if THREADING_DEBUG
    ++lock->reserved1;
#endif
}

void
win32rwUnlockReader(RwLock *lock)
{
    CF_ASSERT_NOT_NULL(lock);
#if THREADING_DEBUG
    --lock->reserved1;
#endif
    ReleaseSRWLockShared((SRWLOCK *)(lock->data));
}

bool
win32rwTryLockWriter(RwLock *lock)
{
    CF_ASSERT_NOT_NULL(lock);
#if THREADING_DEBUG
    return win32tryLockExc((SRWLOCK *)(lock->data), &lock->reserved0);
#else
    return TryAcquireSRWLockExclusive((SRWLOCK *)(lock->data));
#endif
}

void
win32rwLockWriter(RwLock *lock)
{
    CF_ASSERT_NOT_NULL(lock);
#if THREADING_DEBUG
    win32lockExc((SRWLOCK *)(lock->data), &lock->reserved0);
#else
    AcquireSRWLockExclusive((SRWLOCK *)(lock->data));
#endif
}

void
win32rwUnlockWriter(RwLock *lock)
{
    CF_ASSERT_NOT_NULL(lock);
#if THREADING_DEBUG
    win32unlockExc((SRWLOCK *)(lock->data), &lock->reserved0);
#else
    ReleaseSRWLockExclusive((SRWLOCK *)(lock->data));
#endif
}

//------------------------------------------------------------------------------
// ConditionVariable implementation

CF_STATIC_ASSERT(sizeof(((ConditionVariable *)0)->data) == sizeof(CONDITION_VARIABLE),
                 "Invalid ConditionVariable internal size");

static inline bool
win32cvWait(ConditionVariable *cv, SRWLOCK *lock, u32 timeout_ms)
{
    CF_ASSERT_NOT_NULL(cv);
    return SleepConditionVariableSRW((CONDITION_VARIABLE *)(cv->data), lock, timeout_ms, 0);
}

void
win32cvInit(ConditionVariable *cv)
{
    CF_ASSERT_NOT_NULL(cv);
    InitializeConditionVariable((CONDITION_VARIABLE *)(cv->data));
}

void
win32cvShutdown(ConditionVariable *cv)
{
    CF_ASSERT_NOT_NULL(cv);
}

bool
win32cvWaitMutex(ConditionVariable *cv, Mutex *mutex, u32 timeout_ms)
{
    CF_ASSERT_NOT_NULL(mutex);
#if THREADING_DEBUG
    CF_ASSERT(mutex->internal != 0, "Attemped wait on unlocked Mutex");
#endif
    return win32cvWait(cv, (SRWLOCK *)(mutex->data), timeout_ms);
}

bool
win32cvWaitRwLock(ConditionVariable *cv, RwLock *lock, u32 timeout_ms)
{
    CF_ASSERT_NOT_NULL(lock);
#if THREADING_DEBUG
    CF_ASSERT(lock->reserved0 != 0 || lock->reserved1 != 0, "Attemped wait on unlocked RwLock");
#endif
    return win32cvWait(cv, (SRWLOCK *)(lock->data), timeout_ms);
}

void
win32cvSignalOne(ConditionVariable *cv)
{
    CF_ASSERT_NOT_NULL(cv);
    WakeConditionVariable((CONDITION_VARIABLE *)(cv->data));
}

void
win32cvSignalAll(ConditionVariable *cv)
{
    CF_ASSERT_NOT_NULL(cv);
    WakeAllConditionVariable((CONDITION_VARIABLE *)(cv->data));
}
