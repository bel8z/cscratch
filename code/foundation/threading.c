#include "threading.h"
#include "common.h"

#ifdef CF_OS_WIN32

#    pragma warning(push)
#    pragma warning(disable : 5105)

#    define NOMINMAX 1
#    define VC_EXTRALEAN 1
#    define WIN32_LEAN_AND_MEAN 1
#    include <windows.h>
// Must be included AFTER <windows.h>
#    include <process.h>

#    pragma warning(pop)

//------------------------------------------------------------------------------
// Misc implementation

static inline DWORD
win32TimeoutMs(Time timeout)
{
    if (TIME_IS_INFINITE(timeout)) return INFINITE;
    CF_ASSERT(timeout.nanoseconds >= 0, "Negative timeout given");
    I64 ms = timeout.nanoseconds / 1000000;
    return (DWORD)ms;
}

void
threadSleep(Time timeout)
{
    Sleep(win32TimeoutMs(timeout));
}

//------------------------------------------------------------------------------
// Thread implementation

CF_STATIC_ASSERT(sizeof(Thread) == sizeof(HANDLE), "Thread and HANDLE size must be equal");
CF_STATIC_ASSERT(alignof(Thread) == alignof(HANDLE), "Thread must be aligned as HANDLE");

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

Thread
threadCreate(ThreadParms *parms)
{
    CF_ASSERT_NOT_NULL(parms);

    Thread thread = {0};
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
threadDestroy(Thread thread)
{
    if (thread.handle) CloseHandle((HANDLE)thread.handle);
}

bool
threadIsRunning(Thread thread)
{
    DWORD code = 0;
    return (GetExitCodeThread((HANDLE)thread.handle, &code) && code == STILL_ACTIVE);
}

bool
threadWait(Thread thread, Time timeout)
{
    return (thread.handle &&
            WAIT_OBJECT_0 == WaitForSingleObject((HANDLE)thread.handle, win32TimeoutMs(timeout)));
}

bool
threadWaitAll(Thread *threads, Usize num_threads, Time timeout)
{
    CF_ASSERT(num_threads <= U32_MAX, "Given number of threads is too large");
    DWORD count = (DWORD)num_threads;
    DWORD code = WaitForMultipleObjects(count, (HANDLE *)threads, true, win32TimeoutMs(timeout));
    return (code < WAIT_OBJECT_0 + count);
}

//------------------------------------------------------------------------------

// Internal implementation of exclusive locking, shared by Mutex and RwLock

#    if CF_THREADING_DEBUG

static bool
win32tryLockExc(SRWLOCK *lock, U32 *owner_id)
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
win32lockExc(SRWLOCK *lock, U32 *owner_id)
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
win32unlockExc(SRWLOCK *lock, U32 *owner_id)
{
    CF_ASSERT_NOT_NULL(lock);
    CF_ASSERT_NOT_NULL(owner_id);
    *owner_id = 0;
    ReleaseSRWLockExclusive(lock);
}

#    endif

//------------------------------------------------------------------------------
// Mutex implementation

CF_STATIC_ASSERT(sizeof(((Mutex *)0)->data) == sizeof(SRWLOCK), "Invalid Mutex internal size");

void
mutexInit(Mutex *mutex)
{
    CF_ASSERT_NOT_NULL(mutex);
    InitializeSRWLock((SRWLOCK *)(mutex->data));
#    if CF_THREADING_DEBUG
    mutex->internal = 0;
#    endif
}

void
mutexShutdown(Mutex *mutex)
{
    CF_ASSERT_NOT_NULL(mutex);
#    if CF_THREADING_DEBUG
    CF_ASSERT(mutex->internal == 0, "Shutting down an acquired mutex");
#    endif
}

bool
mutexTryAcquire(Mutex *mutex)
{
    CF_ASSERT_NOT_NULL(mutex);
#    if CF_THREADING_DEBUG
    return win32tryLockExc((SRWLOCK *)(mutex->data), &mutex->internal);
#    else
    return TryAcquireSRWLockExclusive((SRWLOCK *)(mutex->data));
#    endif
}

void
mutexAcquire(Mutex *mutex)
{
    CF_ASSERT_NOT_NULL(mutex);
#    if CF_THREADING_DEBUG
    win32lockExc((SRWLOCK *)(mutex->data), &mutex->internal);
#    else
    AcquireSRWLockExclusive((SRWLOCK *)(mutex->data));
#    endif
}

void
mutexRelease(Mutex *mutex)
{
    CF_ASSERT_NOT_NULL(mutex);
#    if CF_THREADING_DEBUG
    win32unlockExc((SRWLOCK *)(mutex->data), &mutex->internal);
#    else
    ReleaseSRWLockExclusive((SRWLOCK *)(mutex->data));
#    endif
}

//------------------------------------------------------------------------------
// RwLock implementation

CF_STATIC_ASSERT(sizeof(((RwLock *)0)->data) == sizeof(SRWLOCK), "Invalid RwLock internal size");

void
rwInit(RwLock *lock)
{
    CF_ASSERT_NOT_NULL(lock);
    InitializeSRWLock((SRWLOCK *)(lock->data));
#    if CF_THREADING_DEBUG
    lock->reserved0 = 0;
    lock->reserved1 = 0;
#    endif
}

void
rwShutdown(RwLock *lock)
{
    CF_ASSERT_NOT_NULL(lock);
#    if CF_THREADING_DEBUG
    CF_ASSERT(lock->reserved0 == 0, "Shutting down an RwLock acquired for writing");
    CF_ASSERT(lock->reserved1 == 0, "Shutting down an RwLock acquired for reading");
#    endif
}

bool
win32rwTryLockReader(RwLock *lock)
{
    CF_ASSERT_NOT_NULL(lock);
    if (TryAcquireSRWLockShared((SRWLOCK *)(lock->data)))
    {
#    if CF_THREADING_DEBUG
        ++lock->reserved1;
#    endif
        return true;
    }
    return false;
}

void
rwLockReader(RwLock *lock)
{
    CF_ASSERT_NOT_NULL(lock);
    AcquireSRWLockShared((SRWLOCK *)(lock->data));
#    if CF_THREADING_DEBUG
    ++lock->reserved1;
#    endif
}

void
rwUnlockReader(RwLock *lock)
{
    CF_ASSERT_NOT_NULL(lock);
#    if CF_THREADING_DEBUG
    --lock->reserved1;
#    endif
    ReleaseSRWLockShared((SRWLOCK *)(lock->data));
}

bool
rwTryLockWriter(RwLock *lock)
{
    CF_ASSERT_NOT_NULL(lock);
#    if CF_THREADING_DEBUG
    return win32tryLockExc((SRWLOCK *)(lock->data), &lock->reserved0);
#    else
    return TryAcquireSRWLockExclusive((SRWLOCK *)(lock->data));
#    endif
}

void
rwLockWriter(RwLock *lock)
{
    CF_ASSERT_NOT_NULL(lock);
#    if CF_THREADING_DEBUG
    win32lockExc((SRWLOCK *)(lock->data), &lock->reserved0);
#    else
    AcquireSRWLockExclusive((SRWLOCK *)(lock->data));
#    endif
}

void
rwUnlockWriter(RwLock *lock)
{
    CF_ASSERT_NOT_NULL(lock);
#    if CF_THREADING_DEBUG
    win32unlockExc((SRWLOCK *)(lock->data), &lock->reserved0);
#    else
    ReleaseSRWLockExclusive((SRWLOCK *)(lock->data));
#    endif
}

//------------------------------------------------------------------------------
// ConditionVariable implementation

CF_STATIC_ASSERT(sizeof(((ConditionVariable *)0)->data) == sizeof(CONDITION_VARIABLE),
                 "Invalid ConditionVariable internal size");

static inline bool
win32cvWait(ConditionVariable *cv, SRWLOCK *lock, Time timeout)
{
    CF_ASSERT_NOT_NULL(cv);
    return SleepConditionVariableSRW((CONDITION_VARIABLE *)(cv->data), lock,
                                     win32TimeoutMs(timeout), 0);
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
cvWaitMutex(ConditionVariable *cv, Mutex *mutex, Time timeout)
{
    CF_ASSERT_NOT_NULL(mutex);
#    if CF_THREADING_DEBUG
    CF_ASSERT(mutex->internal != 0, "Attemped wait on unlocked Mutex");
#    endif
    return win32cvWait(cv, (SRWLOCK *)(mutex->data), timeout);
}

bool
cvWaitRwLock(ConditionVariable *cv, RwLock *lock, Time timeout)
{
    CF_ASSERT_NOT_NULL(lock);
#    if CF_THREADING_DEBUG
    CF_ASSERT(lock->reserved0 != 0 || lock->reserved1 != 0, "Attemped wait on unlocked RwLock");
#    endif
    return win32cvWait(cv, (SRWLOCK *)(lock->data), timeout);
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

#endif
