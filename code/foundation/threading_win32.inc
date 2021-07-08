#include "core.h"
#include "threading.h"
#include "time.h"

#pragma warning(push)
#pragma warning(disable : 5105)

#define NOMINMAX 1
#define VC_EXTRALEAN 1
#define WIN32_LEAN_AND_MEAN 1
#include <windows.h>
// Must be included AFTER <windows.h>
#include <process.h>

#pragma warning(pop)

//------------------------------------------------------------------------------
// Misc implementation

static inline DWORD
win32TimeoutMs(Time timeout)
{
    if (timeIsInfinite(timeout)) return INFINITE;
    CF_ASSERT(timeout.nanoseconds >= 0, "Negative timeout given");
    I64 ms = timeout.nanoseconds / 1000000;
    return (DWORD)ms;
}

void
cfSleep(Time timeout)
{
    Sleep(win32TimeoutMs(timeout));
}

//------------------------------------------------------------------------------
// Thread implementation

CF_STATIC_ASSERT(sizeof(CfThread) == sizeof(HANDLE), "Thread and HANDLE size must be equal");
CF_STATIC_ASSERT(alignof(CfThread) == alignof(HANDLE), "Thread must be aligned as HANDLE");

typedef struct Win32ThreadArgs
{
    ThreadProc proc;
    void *args;
    bool started;
} Win32ThreadArgs;

static U32 WINAPI
win32threadProc(void *data)
{
    Win32ThreadArgs *parms = data;
    ThreadProc proc = parms->proc;
    void *args = parms->args;

    parms->started = true;

    proc(args);

    _endthreadex(0);

    return 0;
}

CfThread
cfThreadCreate(CfThreadParms *parms)
{
    CF_ASSERT_NOT_NULL(parms);

    CfThread thread = {0};
    Win32ThreadArgs args = {.proc = parms->proc, .args = parms->args, .started = false};

    CF_ASSERT(parms->stack_size <= U32_MAX, "Required stack size is too large");

    thread.handle = _beginthreadex(NULL, (U32)parms->stack_size, win32threadProc, &args,
                                   CREATE_SUSPENDED, NULL);

    if (thread.handle)
    {
        if (parms->debug_name)
        {
            WCHAR buffer[1024];
            I32 size = MultiByteToWideChar(CP_UTF8, 0, parms->debug_name, -1, buffer, 1024);
            CF_ASSERT(size > 0 || size < 1024, "Thread debug name is too long");
            SetThreadDescription((HANDLE)thread.handle, buffer);
        }

        ResumeThread((HANDLE)thread.handle);

        while (!args.started)
        {
            // TODO (Matteo): Use a sync primitive instead of a spin wait?
        }
    }

    return thread;
}

void
cfThreadDestroy(CfThread thread)
{
    if (thread.handle) CloseHandle((HANDLE)thread.handle);
}

bool
cfThreadIsRunning(CfThread thread)
{
    DWORD code = 0;
    return (GetExitCodeThread((HANDLE)thread.handle, &code) && code == STILL_ACTIVE);
}

bool
cfThreadWait(CfThread thread, Time timeout)
{
    return (thread.handle &&
            WAIT_OBJECT_0 == WaitForSingleObject((HANDLE)thread.handle, win32TimeoutMs(timeout)));
}

bool
cfThreadWaitAll(CfThread *threads, Usize num_threads, Time timeout)
{
    CF_ASSERT(num_threads <= U32_MAX, "Given number of threads is too large");
    DWORD count = (DWORD)num_threads;
    DWORD code = WaitForMultipleObjects(count, (HANDLE *)threads, true, win32TimeoutMs(timeout));
    return (code < WAIT_OBJECT_0 + count);
}

//------------------------------------------------------------------------------

// Internal implementation of exclusive locking, shared by Mutex and RwLock

#if CF_THREADING_DEBUG

static bool
win32TryLockExc(SRWLOCK *lock, U32 *owner_id)
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
win32LockExc(SRWLOCK *lock, U32 *owner_id)
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
win32UnlockExc(SRWLOCK *lock, U32 *owner_id)
{
    CF_ASSERT_NOT_NULL(lock);
    CF_ASSERT_NOT_NULL(owner_id);
    *owner_id = 0;
    ReleaseSRWLockExclusive(lock);
}

#endif

//------------------------------------------------------------------------------
// Mutex implementation

CF_STATIC_ASSERT(sizeof(((CfMutex *)0)->data) == sizeof(SRWLOCK), "Invalid Mutex internal size");

void
cfMutexInit(CfMutex *mutex)
{
    CF_ASSERT_NOT_NULL(mutex);
    InitializeSRWLock((SRWLOCK *)(mutex->data));
#if CF_THREADING_DEBUG
    mutex->internal = 0;
#endif
}

void
mutexShutdown(CfMutex *mutex)
{
    CF_ASSERT_NOT_NULL(mutex);
#if CF_THREADING_DEBUG
    CF_ASSERT(mutex->internal == 0, "Shutting down an acquired mutex");
#endif
}

bool
cfMutexTryAcquire(CfMutex *mutex)
{
    CF_ASSERT_NOT_NULL(mutex);
#if CF_THREADING_DEBUG
    return win32TryLockExc((SRWLOCK *)(mutex->data), &mutex->internal);
#else
    return TryAcquireSRWLockExclusive((SRWLOCK *)(mutex->data));
#endif
}

void
cfMutexAcquire(CfMutex *mutex)
{
    CF_ASSERT_NOT_NULL(mutex);
#if CF_THREADING_DEBUG
    win32LockExc((SRWLOCK *)(mutex->data), &mutex->internal);
#else
    AcquireSRWLockExclusive((SRWLOCK *)(mutex->data));
#endif
}

void
cfMutexRelease(CfMutex *mutex)
{
    CF_ASSERT_NOT_NULL(mutex);
#if CF_THREADING_DEBUG
    win32UnlockExc((SRWLOCK *)(mutex->data), &mutex->internal);
#else
    ReleaseSRWLockExclusive((SRWLOCK *)(mutex->data));
#endif
}

//------------------------------------------------------------------------------
// RwLock implementation

CF_STATIC_ASSERT(sizeof(((CfRwLock *)0)->data) == sizeof(SRWLOCK), "Invalid RwLock internal size");

void
cfRwInit(CfRwLock *lock)
{
    CF_ASSERT_NOT_NULL(lock);
    InitializeSRWLock((SRWLOCK *)(lock->data));
#if CF_THREADING_DEBUG
    lock->reserved0 = 0;
    lock->reserved1 = 0;
#endif
}

void
cfRwShutdown(CfRwLock *lock)
{
    CF_ASSERT_NOT_NULL(lock);
#if CF_THREADING_DEBUG
    CF_ASSERT(lock->reserved0 == 0, "Shutting down an RwLock acquired for writing");
    CF_ASSERT(lock->reserved1 == 0, "Shutting down an RwLock acquired for reading");
#endif
}

bool
win32RwTryLockReader(CfRwLock *lock)
{
    CF_ASSERT_NOT_NULL(lock);
    if (TryAcquireSRWLockShared((SRWLOCK *)(lock->data)))
    {
#if CF_THREADING_DEBUG
        ++lock->reserved1;
#endif
        return true;
    }
    return false;
}

void
cfRwLockReader(CfRwLock *lock)
{
    CF_ASSERT_NOT_NULL(lock);
    AcquireSRWLockShared((SRWLOCK *)(lock->data));
#if CF_THREADING_DEBUG
    ++lock->reserved1;
#endif
}

void
cfRwUnlockReader(CfRwLock *lock)
{
    CF_ASSERT_NOT_NULL(lock);
#if CF_THREADING_DEBUG
    --lock->reserved1;
#endif
    ReleaseSRWLockShared((SRWLOCK *)(lock->data));
}

bool
cfRwTryLockWriter(CfRwLock *lock)
{
    CF_ASSERT_NOT_NULL(lock);
#if CF_THREADING_DEBUG
    return win32TryLockExc((SRWLOCK *)(lock->data), &lock->reserved0);
#else
    return TryAcquireSRWLockExclusive((SRWLOCK *)(lock->data));
#endif
}

void
cfRwLockWriter(CfRwLock *lock)
{
    CF_ASSERT_NOT_NULL(lock);
#if CF_THREADING_DEBUG
    win32LockExc((SRWLOCK *)(lock->data), &lock->reserved0);
#else
    AcquireSRWLockExclusive((SRWLOCK *)(lock->data));
#endif
}

void
cfRwUnlockWriter(CfRwLock *lock)
{
    CF_ASSERT_NOT_NULL(lock);
#if CF_THREADING_DEBUG
    win32UnlockExc((SRWLOCK *)(lock->data), &lock->reserved0);
#else
    ReleaseSRWLockExclusive((SRWLOCK *)(lock->data));
#endif
}

//------------------------------------------------------------------------------
// ConditionVariable implementation

CF_STATIC_ASSERT(sizeof(((CfConditionVariable *)0)->data) == sizeof(CONDITION_VARIABLE),
                 "Invalid CfConditionVariable internal size");

static inline bool
win32cvWait(CfConditionVariable *cv, SRWLOCK *lock, Time timeout)
{
    CF_ASSERT_NOT_NULL(cv);
    return SleepConditionVariableSRW((CONDITION_VARIABLE *)(cv->data), lock,
                                     win32TimeoutMs(timeout), 0);
}

void
cfCvInit(CfConditionVariable *cv)
{
    CF_ASSERT_NOT_NULL(cv);
    InitializeConditionVariable((CONDITION_VARIABLE *)(cv->data));
}

void
cfCvShutdown(CfConditionVariable *cv)
{
    CF_ASSERT_NOT_NULL(cv);
}

bool
cfCvWaitMutex(CfConditionVariable *cv, CfMutex *mutex, Time timeout)
{
    CF_ASSERT_NOT_NULL(mutex);
#if CF_THREADING_DEBUG
    CF_ASSERT(mutex->internal != 0, "Attemped wait on unlocked Mutex");
#endif
    return win32cvWait(cv, (SRWLOCK *)(mutex->data), timeout);
}

bool
cfCvWaitRwLock(CfConditionVariable *cv, CfRwLock *lock, Time timeout)
{
    CF_ASSERT_NOT_NULL(lock);
#if CF_THREADING_DEBUG
    CF_ASSERT(lock->reserved0 != 0 || lock->reserved1 != 0, "Attemped wait on unlocked RwLock");
#endif
    return win32cvWait(cv, (SRWLOCK *)(lock->data), timeout);
}

void
cfCvSignalOne(CfConditionVariable *cv)
{
    CF_ASSERT_NOT_NULL(cv);
    WakeConditionVariable((CONDITION_VARIABLE *)(cv->data));
}

void
cfCvSignalAll(CfConditionVariable *cv)
{
    CF_ASSERT_NOT_NULL(cv);
    WakeAllConditionVariable((CONDITION_VARIABLE *)(cv->data));
}
