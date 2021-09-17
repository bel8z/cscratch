#include "atom.h"
#include "core.h"
#include "threading.h"
#include "time.h"
#include "win32.h"

//------------------------------------------------------------------------------
// Misc implementation

static inline DWORD
win32DurationMs(Duration duration)
{
    if (timeDurationIsInfinite(duration)) return INFINITE;
    CF_ASSERT(duration.seconds >= 0, "Negative duration given");
    I64 ms = duration.seconds * 1000 + duration.nanos / 1000000;
    return (DWORD)ms;
}

void
cfSleep(Duration duration)
{
    Sleep(win32DurationMs(duration));
}

//------------------------------------------------------------------------------
// Thread implementation

CF_STATIC_ASSERT(sizeof(CfThread) == sizeof(HANDLE), "Thread and HANDLE size must be equal");
CF_STATIC_ASSERT(alignof(CfThread) == alignof(HANDLE), "Thread must be aligned as HANDLE");

typedef struct Win32ThreadData
{
    CfThreadProc proc;
    void *args;

    // Data for synchronization with the creation routine
    SRWLOCK sync;
    CONDITION_VARIABLE signal;
    // NOTE (Matteo): This is a char for compliance with the Interlocked API
    AtomU32 started;

} Win32ThreadData;

static U32 WINAPI
win32threadProc(void *data_ptr)
{
    Win32ThreadData *data = data_ptr;

    // NOTE (Matteo): Copy persistent data locally, so that the containing struct can be freed.
    CfThreadProc proc = data->proc;
    void *args = data->args;

    // NOTE (Matteo): Signal that the thread is started so the creation routine can complete
    AcquireSRWLockExclusive(&data->sync);
    atomFetchOr(&data->started, 1);
    ReleaseSRWLockExclusive(&data->sync);
    WakeConditionVariable(&data->signal);

    proc(args);

    _endthreadex(0);

    return 0;
}

CfThread
cfThreadCreate(CfThreadParms *parms)
{
    CF_ASSERT_NOT_NULL(parms);

    CfThread thread = {0};

    Win32ThreadData data = {
        .proc = parms->proc,
        .args = parms->args,
    };

    InitializeSRWLock(&data.sync);
    InitializeConditionVariable(&data.signal);

    CF_ASSERT(parms->stack_size <= U32_MAX, "Required stack size is too large");

    // NOTE (Matteo): Start the thread suspended so that it can be synchronized with this routine
    thread.handle = _beginthreadex(NULL, (U32)parms->stack_size, win32threadProc, &data,
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

        // NOTE (Matteo): Request thread start and wait until this actually happens
        ResumeThread((HANDLE)thread.handle);
        while (atomFetchInc(&data.started) != 1)
        {
            AcquireSRWLockExclusive(&data.sync);
            SleepConditionVariableSRW(&data.signal, &data.sync, INFINITE, 0);
            ReleaseSRWLockExclusive(&data.sync);
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
cfThreadWait(CfThread thread, Duration duration)
{
    return (thread.handle &&
            WAIT_OBJECT_0 == WaitForSingleObject((HANDLE)thread.handle, win32DurationMs(duration)));
}

bool
cfThreadWaitAll(CfThread *threads, Usize num_threads, Duration duration)
{
    CF_ASSERT(num_threads <= U32_MAX, "Given number of threads is too large");
    DWORD count = (DWORD)num_threads;
    DWORD code = WaitForMultipleObjects(count, (HANDLE *)threads, true, win32DurationMs(duration));
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
    CF_ASSERT(lock->reserved0 == 0, "Shutting down a read/write lock acquired for writing");
    CF_ASSERT(lock->reserved1 == 0, "Shutting down a read/write lock acquired for reading");
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
win32cvWait(CfConditionVariable *cv, SRWLOCK *lock, Duration duration)
{
    CF_ASSERT_NOT_NULL(cv);
    return SleepConditionVariableSRW((CONDITION_VARIABLE *)(cv->data), lock,
                                     win32DurationMs(duration), 0);
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
cfCvWaitMutex(CfConditionVariable *cv, CfMutex *mutex, Duration duration)
{
    CF_ASSERT_NOT_NULL(mutex);
#if CF_THREADING_DEBUG
    CF_ASSERT(mutex->internal != 0, "Attempted wait on unlocked mutex");
#endif
    return win32cvWait(cv, (SRWLOCK *)(mutex->data), duration);
}

bool
cfCvWaitRwLock(CfConditionVariable *cv, CfRwLock *lock, Duration duration)
{
    CF_ASSERT_NOT_NULL(lock);
#if CF_THREADING_DEBUG
    CF_ASSERT(lock->reserved0 != 0 || lock->reserved1 != 0,
              "Attempted wait on unlocked read/write lock");
#endif
    return win32cvWait(cv, (SRWLOCK *)(lock->data), duration);
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

//------------------------------------------------------------------------------
// Semaphore implementation

// NOTE (Matteo): Lightweight semaphore with partial spinning based on
// https://preshing.com/20150316/semaphores-are-surprisingly-versatile/

typedef struct RawSemaphore
{
    HANDLE handle;
    AtomI32 count;
} RawSemaphore;

// TODO (Matteo): Tweak spin count
#define SEMA_SPIN_COUNT 10000

CF_STATIC_ASSERT(sizeof(CfSemaphore) >= sizeof(RawSemaphore), "Invalid CfSemaphore internal size");

CF_API void
cfSemaInit(CfSemaphore *sema, I32 init_count)
{
    RawSemaphore *self = (RawSemaphore *)sema->data;

    self->handle = CreateSemaphore(NULL, 0, MAXLONG, NULL);
    atomWrite(&self->count, init_count);
}

CF_API bool
cfSemaTryWait(CfSemaphore *sema)
{
    RawSemaphore *self = (RawSemaphore *)sema->data;
    I32 prev_count = atomRead(&self->count);
    atomAcquireFence();
    return (prev_count > 0 && atomCompareExchange(&self->count, prev_count, prev_count - 1));
}

CF_API void
cfSemaWait(CfSemaphore *sema)
{
    RawSemaphore *self = (RawSemaphore *)sema->data;

    I32 prev_count;
    I32 spin_count = SEMA_SPIN_COUNT;

    while (spin_count--)
    {
        prev_count = atomRead(&self->count);
        atomAcquireFence();
        if (prev_count > 0 && atomCompareExchange(&self->count, prev_count, prev_count - 1))
        {
            return;
        }
        // Prevent compiler from collapsing loop
        atomAcquireCompFence();
    }

    prev_count = atomFetchDec(&self->count);
    atomAcquireFence();
    if (prev_count <= 0) WaitForSingleObject(self->handle, INFINITE);
}

CF_API void
cfSemaSignalOne(CfSemaphore *sema)
{
    cfSemaSignal(sema, 1);
}

CF_API void
cfSemaSignal(CfSemaphore *sema, I32 count)
{
    RawSemaphore *self = (RawSemaphore *)sema->data;
    atomReleaseFence();
    I32 prev_count = atomFetchAdd(&self->count, count);
    I32 signal_count = -prev_count < count ? -prev_count : count;
    if (signal_count > 0)
    {
        ReleaseSemaphore(self->handle, count, NULL);
    }
}
