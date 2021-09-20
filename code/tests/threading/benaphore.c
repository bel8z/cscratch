#include "benaphore.h"

#include "foundation/atom.inl"

void
benaInit(Benaphore *bena)
{
    cfSemaInit(&bena->sema, 0);
    atomInit(&bena->contention_count, 0);
}

bool
benaTryLock(Benaphore *bena)
{
    if (atomRead(&bena->contention_count) != 0) return false;

    if (atomCompareExchange(&bena->contention_count, 0, 1) == 0)
    {
        atomAcquireFence();
        return true;
    }

    return false;
}

void
benaLock(Benaphore *bena)
{
    if (atomFetchInc(&bena->contention_count) > 0)
    {
        atomAcquireFence();
        cfSemaWait(&bena->sema);
    }
}

void
benaRelease(Benaphore *bena)
{
    atomReleaseFence();
    I32 old_count = atomFetchDec(&bena->contention_count);

    CF_ASSERT(old_count > 0, "");

    if (old_count > 1)
    {
        cfSemaSignalOne(&bena->sema);
    }
}
