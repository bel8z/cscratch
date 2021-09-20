#pragma once

#include "foundation/atom.h"
#include "foundation/threading.h"

/// "Benaphore" is a lightweight mutex implemented using an atomic counter and
/// a semaphore
typedef struct Benaphore
{
    CfSemaphore sema;
    AtomI32 contention_count;
} Benaphore;

void benaInit(Benaphore *bena);
bool benaTryLock(Benaphore *bena);
void benaLock(Benaphore *bena);
void benaRelease(Benaphore *bena);
