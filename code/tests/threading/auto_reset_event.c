#include "auto_reset_event.h"

#include "foundation/atom.inl"

// Auto-reset event implementation

// NOTE (Matteo):  https://preshing.com/20150316/semaphores-are-surprisingly-versatile/

CF_API void
cfArEventInit(CfAutoResetEvent *event)
{
    atomInit(&event->status, 0);
    cfSemaInit(&event->sema, 0);
}

CF_API void
cfArEventWait(CfAutoResetEvent *event)
{
    I32 prev_status = atomRead(&event->status);

    for (;;)
    {
        CF_ASSERT(prev_status <= 1, "");
        I32 next_status = prev_status < 1 ? prev_status + 1 : 1;

        atomReleaseFence();
        if (!atomCompareExchangeWeak(&event->status, &prev_status, next_status))
        {
            // The compare-exchange failed, likely because another thread changed status.
            // prev_status has been updated. Retry the CAS loop.
        }
        else
        {
            break;
        }
    }

    if (prev_status < 0)
    {
        // Release one waiting thread.
        cfSemaSignalOne(&event->sema);
    }
}

CF_API void
cfArEventSignal(CfAutoResetEvent *event)
{
    I32 prev_status = atomFetchDec(&event->status);
    atomAcquireFence();
    CF_ASSERT(prev_status <= 1, "");
    if (prev_status < 1)
    {
        cfSemaWait(&event->sema);
    }
}
