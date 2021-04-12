#ifndef CF_ALLOCATOR_H

#include "common.h"

#define CF_ALLOCATOR_FUNC(name) \
    void *name(void *state, void *memory, usize old_size, usize new_size)

typedef CF_ALLOCATOR_FUNC(AllocatorFunc);

typedef struct CfAllocator
{
    void *state;
    AllocatorFunc *reallocate;
} CfAllocator;

#define CF_ALLOCATE(a, size) a->reallocate(a->state, NULL, 0, size)

#define CF_REALLOCATE(a, memory, old_size, new_size) \
    a->reallocate(a->state, memory, old_size, new_size)

#define CF_DEALLOCATE(a, memory, size) a->reallocate(a->state, memory, size, 0)

#define CF_ALLOCATOR_H
#endif
