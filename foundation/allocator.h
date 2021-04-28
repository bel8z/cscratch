#ifndef CF_ALLOCATOR_H

#include "common.h"

typedef struct cfAllocator cfAllocator;
typedef struct cfAllocatorStats cfAllocatorStats;

#define CF_ALLOCATOR_FUNC(name) \
    void *name(void *state, void *memory, usize old_size, usize new_size)

#define CF_ALLOC_STATS_FUNC(name) cfAllocatorStats name(void *state)

typedef CF_ALLOCATOR_FUNC(cfAllocatorFunc);
typedef CF_ALLOC_STATS_FUNC(cfAllocStatsFunc);

struct cfAllocatorStats
{
    usize count;
    usize size;
};

struct cfAllocator
{
    void *state;
    cfAllocatorFunc *reallocate;
    cfAllocStatsFunc *stats;
};

#define cfAlloc(a, size) a->reallocate(a->state, NULL, 0, size)

#define cfRealloc(a, memory, old_size, new_size) a->reallocate(a->state, memory, old_size, new_size)

#define cfFree(a, memory, size) a->reallocate(a->state, memory, size, 0)

#define cfAllocStats(a) a->stats(a->state)

#define CF_ALLOCATOR_H
#endif
