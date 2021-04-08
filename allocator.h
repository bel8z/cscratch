#ifndef ALLOCATOR_H

#include "common.h"

#define ALLOCATOR_FUNC(name) void *name(void *state, void *memory, usize old_size, usize new_size)

typedef ALLOCATOR_FUNC(AllocatorFunc);

typedef struct Allocator
{
    void *state;
    AllocatorFunc *reallocate;
} Allocator;

#define ALLOCATE(a, size) a->reallocate(a->state, NULL, 0, size)

#define REALLOCATE(a, memory, old_size, new_size) \
    a->reallocate(a->state, memory, old_size, new_size)

#define DEALLOCATE(a, memory, size) a->reallocate(a->state, memory, size, 0)



#define ALLOCATOR_H
#endif
