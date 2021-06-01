#include "win32.h"

#include "foundation/common.h"
#include "foundation/threading.h"

//------------------------------------------------------------------------------

CF_STATIC_ASSERT(sizeof(Thread) == sizeof(HANDLE), "Thread and HANDLE size must be equal");
CF_STATIC_ASSERT(alignof(Thread) == alignof(HANDLE), "Thread must be aligned as HANDLE");

CF_STATIC_ASSERT(sizeof(((Mutex *)0)->data) == sizeof(SRWLOCK), "Invalid Mutex internal size");

CF_STATIC_ASSERT(sizeof(((RwLock *)0)->data) == sizeof(SRWLOCK), "Invalid RwLock internal size");

CF_STATIC_ASSERT(sizeof(((ConditionVariable *)0)->data) == sizeof(CONDITION_VARIABLE),
                 "Invalid ConditionVariable internal size");

//------------------------------------------------------------------------------

static void
win32Sleep(u32 ms)
{
    Sleep(ms);
}

//------------------------------------------------------------------------------

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
win32ThreadCreate(ThreadParms *parms)
{
    Thread thread = {0};

    // TODO (Matteo): Use dedicated, lighter, data structure?
    ThreadParms *parms_copy = HeapAlloc(GetProcessHeap(), 0, sizeof(*parms_copy));

    if (parms_copy)
    {
        *parms_copy = *parms;

        CF_ASSERT(parms_copy->stack_size <= U32_MAX, "Required stack size is too large");

        thread.handle = _beginthreadex(NULL, (u32)parms_copy->stack_size, win32ThreadProc,
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

static void
win32ThreadDestroy(Thread thread)
{
    if (thread.handle) CloseHandle((HANDLE)thread.handle);
}

static bool
win32ThreadIsRunning(Thread thread)
{
    DWORD code = 0;
    return (GetExitCodeThread((HANDLE)thread.handle, &code) && code == STILL_ACTIVE);
}

static bool
win32ThreadWait(Thread thread, u32 timeout_ms)
{
    return (thread.handle &&
            WAIT_OBJECT_0 == WaitForSingleObject((HANDLE)thread.handle, timeout_ms));
}

static bool
win32ThreadWaitAll(Thread *threads, usize num_threads, u32 timeout_ms)
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
win32TryLockExc(SRWLOCK *lock, u32 *owner_id)
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
win32LockExc(SRWLOCK *lock, u32 *owner_id)
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
win32UnlockExc(SRWLOCK *lock, u32 *owner_id)
{
    CF_ASSERT_NOT_NULL(lock);
    CF_ASSERT_NOT_NULL(owner_id);
    *owner_id = 0;
    ReleaseSRWLockExclusive(lock);
}

#endif

//------------------------------------------------------------------------------

static void
win32MutexInit(Mutex *mutex)
{
    CF_ASSERT_NOT_NULL(mutex);
    InitializeSRWLock((SRWLOCK *)(mutex->data));
}

void
win32MutexShutdown(Mutex *mutex)
{
    CF_ASSERT_NOT_NULL(mutex);
#if THREADING_DEBUG
    CF_ASSERT(mutex->internal == 0, "Shutting down an acquired mutex");
#endif
}

static bool
win32MutexTryAcquire(Mutex *mutex)
{
    CF_ASSERT_NOT_NULL(mutex);
#if THREADING_DEBUG
    return win32TryLockExc((SRWLOCK *)(mutex->data), &mutex->internal);
#else
    return TryAcquireSRWLockExclusive((SRWLOCK *)(mutex->data));
#endif
}

static void
win32MutexAcquire(Mutex *mutex)
{
    CF_ASSERT_NOT_NULL(mutex);
#if THREADING_DEBUG
    win32LockExc((SRWLOCK *)(mutex->data), &mutex->internal);
#else
    AcquireSRWLockExclusive((SRWLOCK *)(mutex->data));
#endif
}

static void
win32MutexRelease(Mutex *mutex)
{
    CF_ASSERT_NOT_NULL(mutex);
#if THREADING_DEBUG
    win32UnlockExc((SRWLOCK *)(mutex->data), &mutex->internal);
#else
    ReleaseSRWLockExclusive((SRWLOCK *)(mutex->data));
#endif
}

//------------------------------------------------------------------------------
static void
win32RwInit(RwLock *lock)
{
    CF_ASSERT_NOT_NULL(lock);
    InitializeSRWLock((SRWLOCK *)(lock->data));
}

static void
win32RwShutdown(RwLock *lock)
{
    CF_ASSERT_NOT_NULL(lock);
#if THREADING_DEBUG
    CF_ASSERT(lock->reserved0 == 0, "Shutting down an RwLock acquired for writing");
    CF_ASSERT(lock->reserved1 == 0, "Shutting down an RwLock acquired for reading");
#endif
}

static bool
win32RwTryLockReader(RwLock *lock)
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

static void
win32RwLockReader(RwLock *lock)
{
    CF_ASSERT_NOT_NULL(lock);
    AcquireSRWLockShared((SRWLOCK *)(lock->data));
#if THREADING_DEBUG
    ++lock->reserved1;
#endif
}

static void
win32RwUnlockReader(RwLock *lock)
{
    CF_ASSERT_NOT_NULL(lock);
#if THREADING_DEBUG
    --lock->reserved1;
#endif
    ReleaseSRWLockShared((SRWLOCK *)(lock->data));
}

static bool
win32RwTryLockWriter(RwLock *lock)
{
    CF_ASSERT_NOT_NULL(lock);
#if THREADING_DEBUG
    return win32TryLockExc((SRWLOCK *)(lock->data), &lock->reserved0);
#else
    return TryAcquireSRWLockExclusive((SRWLOCK *)(lock->data));
#endif
}

static void
win32RwLockWriter(RwLock *lock)
{
    CF_ASSERT_NOT_NULL(lock);
#if THREADING_DEBUG
    win32LockExc((SRWLOCK *)(lock->data), &lock->reserved0);
#else
    AcquireSRWLockExclusive((SRWLOCK *)(lock->data));
#endif
}

static void
win32RwUnlockWriter(RwLock *lock)
{
    CF_ASSERT_NOT_NULL(lock);
#if THREADING_DEBUG
    win32UnlockExc((SRWLOCK *)(lock->data), &lock->reserved0);
#else
    ReleaseSRWLockExclusive((SRWLOCK *)(lock->data));
#endif
}

//------------------------------------------------------------------------------

static inline bool
win32CvWait(ConditionVariable *cv, SRWLOCK *lock, u32 timeout_ms)
{
    CF_ASSERT_NOT_NULL(cv);
    return SleepConditionVariableSRW((CONDITION_VARIABLE *)(cv->data), lock, timeout_ms, 0);
}

static void
win32CvInit(ConditionVariable *cv)
{
    CF_ASSERT_NOT_NULL(cv);
    InitializeConditionVariable((CONDITION_VARIABLE *)(cv->data));
}

static void
win32CvShutdown(ConditionVariable *cv)
{
    CF_ASSERT_NOT_NULL(cv);
}

static bool
win32CvWaitMutex(ConditionVariable *cv, Mutex *mutex, u32 timeout_ms)
{
    CF_ASSERT_NOT_NULL(mutex);
#if THREADING_DEBUG
    CF_ASSERT(mutex->internal != 0, "Attemped wait on unlocked Mutex");
#endif
    return win32CvWait(cv, (SRWLOCK *)(mutex->data), timeout_ms);
}

static bool
win32CvWaitRwLock(ConditionVariable *cv, RwLock *lock, u32 timeout_ms)
{
    CF_ASSERT_NOT_NULL(lock);
#if THREADING_DEBUG
    CF_ASSERT(lock->reserved0 != 0 || lock->reserved1 != 0, "Attemped wait on unlocked RwLock");
#endif
    return win32CvWait(cv, (SRWLOCK *)(lock->data), timeout_ms);
}

static void
win32CvSignalOne(ConditionVariable *cv)
{
    CF_ASSERT_NOT_NULL(cv);
    WakeConditionVariable((CONDITION_VARIABLE *)(cv->data));
}

static void
win32CvSignalAll(ConditionVariable *cv)
{
    CF_ASSERT_NOT_NULL(cv);
    WakeAllConditionVariable((CONDITION_VARIABLE *)(cv->data));
}
