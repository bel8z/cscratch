#include "threading.h"

//------------------------------------------------------------------------------
// OS primitives implementation

#ifdef CF_OS_WIN32
#    include "threading_win32.c"
#else
#    error "Threading API not implemented for this platform"
#endif

//------------------------------------------------------------------------------
// Semaphore implementation

// NOTE (Matteo): Lightweight semaphore with partial spinning based on
// https://preshing.com/20150316/semaphores-are-surprisingly-versatile/

// TODO (Matteo): Tweak spin count
#define SEMA_SPIN_COUNT 10000

static CfSemaphoreHandle semaHandleCreate(Size init_count);
static void semaHandleWait(CfSemaphoreHandle handle);
static void semaHandleSignal(CfSemaphoreHandle handle, Size count);

CF_API void
cfSemaInit(CfSemaphore *sema, Size init_count)
{
#if SEMA_SPIN_COUNT != 0
    sema->handle = semaHandleCreate(0);
    atomWrite(&sema->count, init_count);
#else
    sema->handle = semaHandleCreate(init_count);
#endif
}

CF_API bool
cfSemaTryWait(CfSemaphore *sema)
{
#if SEMA_SPIN_COUNT != 0
    Offset prev_count = atomRead(&sema->count);
    if (prev_count > 0 &&
        prev_count == atomCompareExchange(&sema->count, prev_count, prev_count - 1))
    {
        atomAcquireFence();
        return true;
    }
#endif
    return false;
}

CF_API void
cfSemaWait(CfSemaphore *sema)
{
#if SEMA_SPIN_COUNT != 0
    Offset prev_count;
    Offset spin_count = SEMA_SPIN_COUNT;

    while (spin_count--)
    {
        prev_count = atomRead(&sema->count);
        if (prev_count > 0 &&
            prev_count == atomCompareExchange(&sema->count, prev_count, prev_count - 1))
        {
            atomAcquireFence();
            return;
        }
        // Prevent compiler from collapsing loop
        atomAcquireCompFence();
    }

    prev_count = atomFetchDec(&sema->count);
    atomAcquireFence();
    if (prev_count <= 0) semaHandleWait(sema->handle);
#else
    semaHandleWait(sema->handle);
#endif
}

CF_API void
cfSemaSignalOne(CfSemaphore *sema)
{
    cfSemaSignal(sema, 1);
}

CF_API void
cfSemaSignal(CfSemaphore *sema, Size count)
{
#if SEMA_SPIN_COUNT != 0
    atomReleaseFence();
    Offset prev_count = atomFetchAdd(&sema->count, count);
    Offset signal_count = -prev_count < count ? -prev_count : count;
    if (signal_count > 0)
    {
        semaHandleSignal(sema->handle, count);
    }
#else
    semaHandleSignal(sema->handle, count);
#endif
}
