#ifndef CF_ALLOCATOR_H

#include "common.h"

typedef struct CfAllocator CfAllocator;
typedef struct CfAllocatorStats CfAllocatorStats;

#define CF_ALLOCATOR_FUNC(name) \
    void *name(void *state, void *memory, usize old_size, usize new_size)

#define CF_ALLOC_STATS_FUNC(name) CfAllocatorStats name(void *state)

typedef CF_ALLOCATOR_FUNC(CfAllocatorFunc);
typedef CF_ALLOC_STATS_FUNC(CfAllocStatsFunc);

struct CfAllocatorStats
{
    usize count;
    usize size;
};

struct CfAllocator
{
    void *state;
    CfAllocatorFunc *reallocate;
    CfAllocStatsFunc *stats;
};

#define CF_ALLOCATE(a, size) a->reallocate(a->state, NULL, 0, size)

#define CF_REALLOCATE(a, memory, old_size, new_size) \
    a->reallocate(a->state, memory, old_size, new_size)

#define CF_DEALLOCATE(a, memory, size) a->reallocate(a->state, memory, size, 0)

#define CF_ALLOCATOR_H
#endif
