#include "foundation/common.h"

#include <stdio.h>

#pragma warning(push)
#pragma warning(disable : 5105)

#define NOMINMAX 1
#define VC_EXTRALEAN 1
#define WIN32_LEAN_AND_MEAN 1
#include <windows.h>
// Must be included AFTER <windows.h>
#include <commdlg.h>
#include <process.h>
#include <shellapi.h>

#pragma warning(pop)

//------------------------------------------------------------------------------

typedef struct Mutex
{
    u8 data[sizeof(void *)];
} Mutex;

void mutexInit(Mutex *mutex);
void mutexAcquire(Mutex *mutex);
void mutexRelease(Mutex *mutex);

//------------------------------------------------------------------------------

typedef struct Thread
{
    uptr handle;
} Thread;

typedef void (*ThreadProc)(void *data);

typedef struct ThreadParms
{
    ThreadProc proc;
    void *args;
    usize stack_size;
    bool suspended;
} ThreadParms;

Thread threadCreate(ThreadParms *parms);
void threadDestroy(Thread thread);
void threadStart(Thread thread);
bool threadIsRunning(Thread thread);
bool threadWait(Thread thread, u32 ms);
bool threadWaitAll(Thread *threads, usize num_threads, u32 ms);

//------------------------------------------------------------------------------

CF_STATIC_ASSERT(sizeof(Thread) == sizeof(HANDLE), "Thread and HANDLE size must be equal");
CF_STATIC_ASSERT(alignof(Thread) == alignof(HANDLE), "Thread must be aligned as HANDLE");
CF_STATIC_ASSERT(sizeof(((Mutex *)0)->data) == sizeof(SRWLOCK), "Invalid mutex internal size");

u32 WINAPI
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

Thread
threadCreate(ThreadParms *parms)
{
    Thread thread = {0};
    ThreadParms *parms_copy = HeapAlloc(GetProcessHeap(), 0, sizeof(*parms_copy));

    if (parms_copy)
    {
        *parms_copy = *parms;

        CF_ASSERT(parms_copy->stack_size <= U32_MAX, "Required stack size is too large");

        u32 flags = parms_copy->suspended ? CREATE_SUSPENDED : 0;
        thread.handle = _beginthreadex(NULL, (u32)parms_copy->stack_size, win32ThreadProc,
                                       parms_copy, flags, NULL);
    }

    return thread;
}

void
threadDestroy(Thread thread)
{
    if (thread.handle)
    {
        CloseHandle((HANDLE)thread.handle);
    }
}

void
threadStart(Thread thread)
{
    if (thread.handle)
    {
        ResumeThread((HANDLE)thread.handle);
    }
}

bool
threadIsRunning(Thread thread)
{
    DWORD code = 0;
    return (GetExitCodeThread((HANDLE)thread.handle, &code) && code == STILL_ACTIVE);
}

bool
threadWait(Thread thread, u32 ms)
{
    return (thread.handle && WAIT_OBJECT_0 == WaitForSingleObject((HANDLE)thread.handle, ms));
}

bool
threadWaitAll(Thread *threads, usize num_threads, u32 ms)
{
    CF_ASSERT(num_threads <= U32_MAX, "Given number of threads is too large");
    DWORD count = (DWORD)num_threads;
    DWORD code = WaitForMultipleObjects(count, (HANDLE *)threads, true, ms);
    return (code < WAIT_OBJECT_0 + count);
}

//------------------------------------------------------------------------------

void
mutexInit(Mutex *mutex)
{
    CF_ASSERT_NOT_NULL(mutex);
    InitializeSRWLock((SRWLOCK *)(mutex->data));
}

void
mutexAcquire(Mutex *mutex)
{
    CF_ASSERT_NOT_NULL(mutex);
    AcquireSRWLockExclusive((SRWLOCK *)(mutex->data));
}

void
mutexRelease(Mutex *mutex)
{
    CF_ASSERT_NOT_NULL(mutex);
    ReleaseSRWLockExclusive((SRWLOCK *)(mutex->data));
}

//------------------------------------------------------------------------------

void
myThreadProc(void *parm)
{
    CF_UNUSED(parm);

    for (i32 i = 10; i >= 0; --i)
    {
        fprintf(stdout, "\r%d", i);
        fflush(stdout);
        Sleep(1000);
    }
}

int
main(void)
{
    Thread thread = threadCreate(&(ThreadParms){.proc = myThreadProc, .suspended = true});

    printf("\nThread created\n");

    threadStart(thread);

    printf("\nThread started\n");

    while (threadIsRunning(thread))
    {
        Sleep(15);
    }

    printf("\nThread terminated\n");

    return 0;
}
