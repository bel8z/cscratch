#ifndef VM_ARENA_H

// Dependencies
#include "common.h"

typedef struct cfVirtualMemory cfVirtualMemory;

/// Linear arena allocator that can use either a fixed size buffer or virtual
/// memory as a backing store
typedef struct Arena
{
    U32 reserved;
    U32 allocated;
    U8 *memory;
    cfVirtualMemory *vm;
} Arena;

typedef struct ArenaTempState
{
    Arena *arena;
    U32 allocated;
} ArenaTempState;

/// Initialize the arena by reserving a block of virtual memory of the required size
bool arenaInitVm(Arena *arena, cfVirtualMemory *vm, U32 reserved_size);

/// Initialize the arena with a fixed size buffer
void arenaInitBuffer(Arena *arena, U8 *buffer, U32 buffer_size);

/// Free all the memory allocated by the arena and render it unable to provide
/// any memory after this call.
void arenaShutdown(Arena *arena);

/// Free all the memory allocated by the arena. In case of a virtual memory backing
/// store, the memory is decommitted (returned to the OS)
void arenaFreeAll(Arena *arena);

/// Allocate a block of the given size and alignment from the top of the arena stack
void *arenaAllocAlign(Arena *arena, U32 size, U32 alignment);

/// Allocate a block of the given size and default alignment from the top of the arena stack
inline void *
arenaAlloc(Arena *arena, U32 size)
{
    return arenaAllocAlign(arena, size, CF_MAX_ALIGN);
}

/// Try reallocating a block of the given size and alignment at the top of the arena stack
/// If the block does not match the last allocation, a new block is allocated
void *arenaReallocAlign(Arena *arena, void *memory, U32 old_size, U32 new_size, U32 alignment);

/// Try reallocating a block of the given size and default alignment at the top of the arena stack
/// If the block does not match the last allocation, a new block is allocated
inline void *
arenaRealloc(Arena *arena, void *memory, U32 old_size, U32 new_size)
{
    return arenaReallocAlign(arena, memory, old_size, new_size, CF_MAX_ALIGN);
}

/// Returns a block of the given size to the top of the arena stack; this is
/// effective only if the block matches with the last allocation
void arenaFree(Arena *arena, void const *memory, U32 size);

/// Allocates a block which fits the given struct on top of the arena stack
#define arenaAllocStruct(arena, Type) (Type *)arenaAllocAlign(arena, sizeof(Type), alignof(Type))

/// Try freeing a block which fits the given struct on top of the arena stack
#define arenaFreeStruct(arena, Type, ptr) arenaFree(arena, ptr, sizeof(Type))

/// Allocates a block which fits the given array on top of the arena stack
#define arenaAllocArray(arena, Type, count) \
    (Type *)arenaAllocAlign(arena, count * sizeof(Type), alignof(Type))

/// Allocates a block which fits the given array on top of the arena stack
#define arenaReallocArray(arena, Type, array, old_count, new_count)                             \
    (Type *)arenaReallocAlign(arena, array, old_count * sizeof(Type), new_count * sizeof(Type), \
                              alignof(Type))

/// Try freeing a block which fits the given array on top of the arena stack
#define arenaFreeArray(arena, Type, count, ptr) arenaFree(arena, ptr, count * sizeof(Type))

ArenaTempState arenaSave(Arena *arena);
void arenaRestore(Arena *arena, ArenaTempState);

#define ARENA_TEMP_BEGIN(arena) ArenaTempState ARENA_TEMP_END_NOT_CALLED = arenaSave(arena)
#define ARENA_TEMP_END(arena) arenaRestore(arena, ARENA_TEMP_END_NOT_CALLED)

#define VM_ARENA_H
#endif
