#pragma once

#include "common.h"

/// Definition of the main allocation function
#define CF_ALLOCATOR_FUNC(name) \
    void *name(void *state, void *memory, Usize old_size, Usize new_size)

/// Generic allocator interface
/// The memory provided by this interface should already be cleared to 0
struct cfAllocator
{
    void *state;
    CF_ALLOCATOR_FUNC((*reallocate));
};

// Helper macro-functions

#define cfAlloc(a, size) (a)->reallocate((a)->state, NULL, 0, size)

#define cfRealloc(a, memory, old_size, new_size) \
    (a)->reallocate((a)->state, (memory), (old_size), (new_size))

#define cfFree(a, memory, size) (a)->reallocate((a)->state, (void *)(memory), (size), 0)
