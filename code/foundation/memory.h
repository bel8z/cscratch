#pragma once

/// Foundation memory services
/// This is not an API header, include it in implementation files only

#include "core.h"

//----------------------------//
//   Basic memory utilities   //
//----------------------------//

#define cfMemAlloc(a, size) cfMemAllocAlign(a, size, CF_MAX_ALIGN)
#define cfMemAllocAlign(a, size, align) (a).func((a).state, NULL, 0, (size), (align))

#define cfMemReallocAlign(a, mem, old_size, new_size, align) \
    (a).func((a).state, (mem), (old_size), (new_size), (align))
#define cfMemRealloc(a, mem, old_size, new_size) \
    cfMemReallocAlign(a, mem, old_size, new_size, CF_MAX_ALIGN)

#define cfMemFree(a, mem, size) (a).func((a).state, (void *)(mem), (size), 0, 0)

CF_API void cfMemClear(void *mem, Usize count);
CF_API void cfMemCopy(void const *from, void *to, Usize count);
CF_API void cfMemCopySafe(void const *from, Usize from_size, void *to, Usize to_size);
CF_API void cfMemWrite(U8 *mem, U8 value, Usize count);
CF_API I32 cfMemCompare(void const *left, void const *right, Usize count);
CF_API bool cfMemMatch(void const *left, void const *right, Usize count);

inline U8 const *
cfMemAlignForward(U8 const *address, Usize alignment)
{
    CF_ASSERT((alignment & (alignment - 1)) == 0, "Alignment is not a power of 2");
    // Same as (address % alignment) but faster as alignment is a power of 2
    Uptr modulo = (Uptr)address & (alignment - 1);
    // Move pointer forward if needed
    return modulo ? address + alignment - modulo : address;
}

#define cfEqual(a, b) (CF_SAME_TYPE(a, b), cfMemMatch(&a, &b, sizeof(a)))

//------------------------//
//   Virtual memory API   //
//------------------------//

// NOTE (Matteo): The implementation of this API must be provided by the platform layer

#define VM_RESERVE_FUNC(name) void *name(Usize size)
#define VM_RELEASE_FUNC(name) void name(void *memory, Usize size)

#define VM_COMMIT_FUNC(name) bool name(void *memory, Usize size)
#define VM_REVERT_FUNC(name) void name(void *memory, Usize size)

typedef struct CfVirtualMemory
{
    VM_RESERVE_FUNC((*reserve));
    VM_RELEASE_FUNC((*release));
    VM_COMMIT_FUNC((*commit));
    VM_REVERT_FUNC((*revert));

    Usize page_size;
} CfVirtualMemory;

#define cfVmReserve(vm, size) (vm)->reserve(size)
#define cfVmRelease(vm, mem, size) (vm)->release(mem, size)
#define cfVmCommit(vm, mem, size) (vm)->commit(mem, size)
#define cfVmRevert(vm, mem, size) (vm)->revert(mem, size)

//------------------//
//   Memory arena   //
//------------------//

/// Linear arena allocator that can use either a fixed size buffer or virtual
/// memory as a backing store
typedef struct Arena
{
    // TODO (Matteo): Use U64 explicitly for sizes?

    Usize reserved;
    Usize allocated;
    Usize committed;
    Usize save_stack;
    U8 *memory;
    CfVirtualMemory *vm;
} Arena;

typedef struct ArenaTempState
{
    Arena *arena;
    Usize allocated;
    Usize stack_id;
} ArenaTempState;

// NOTE (Matteo): On virtual memory usage
// On Windows a VM block must be released by giving the address of the original reservation (the
// size is ignored).
// This doesn't work well with the fact that an arena can be split into smaller ones (which I think
// is a nice feature to have): each sub-arena won't be able to release it's chunk of memory, but
// only the original one can do so (without knowing the full size anymore).
// To patch this abstraction leak (I don't know if the same issue applies on *nix), I decided to
// move the responsibility of reserving and releasing VM outside of the arena, which now works
// always with the memory block you give it (it only needs to commit it in case of VM).

/// Initialize the arena using a reserved block of virtual memory, from which actual pages can be
/// committed
CF_API bool arenaInitOnVm(Arena *arena, CfVirtualMemory *vm, void *reserved_block,
                          Usize reserved_size);

/// Initialize the arena using a pre-allocated memory buffer
CF_API void arenaInitOnBuffer(Arena *arena, U8 *buffer, Usize buffer_size);

/// Allocate a block of virtual memory and initialize an arena directly in it
CF_API Arena *arenaBootstrapFromVm(CfVirtualMemory *vm, void *reserved_block, Usize reserved_size);

// TODO (Matteo): Bootstrap from buffer

/// Free all the memory allocated by the arena. In case of a virtual memory backing
/// store, the memory is decommitted (returned to the OS)
CF_API void arenaClear(Arena *arena);

inline Usize
arenaRemaining(Arena *arena)
{
    return arena->reserved - arena->allocated;
}

/// Allocate a block of the given size and alignment from the top of the arena stack
CF_API void *arenaAllocAlign(Arena *arena, Usize size, Usize alignment);

/// Allocate a block of the given size and default alignment from the top of the arena stack
inline void *
arenaAlloc(Arena *arena, Usize size)
{
    return arenaAllocAlign(arena, size, CF_MAX_ALIGN);
}

/// Try reallocating a block of the given size and alignment at the top of the arena stack
/// If the block does not match the last allocation, a new block is allocated
CF_API void *arenaReallocAlign(Arena *arena, void *memory, Usize old_size, Usize new_size,
                               Usize alignment);

/// Try reallocating a block of the given size and default alignment at the top of the arena stack
/// If the block does not match the last allocation, a new block is allocated
inline void *
arenaRealloc(Arena *arena, void *memory, Usize old_size, Usize new_size)
{
    return arenaReallocAlign(arena, memory, old_size, new_size, CF_MAX_ALIGN);
}

/// Returns a block of the given size to the top of the arena stack; this is
/// effective only if the block matches with the last allocation
CF_API void arenaFree(Arena *arena, void *memory, Usize size);

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

CF_API ArenaTempState arenaSave(Arena *arena);
CF_API void arenaRestore(ArenaTempState state);

CF_API bool arenaSplit(Arena *arena, Arena *split, Usize size);

CF_API CfAllocator arenaAllocator(Arena *arena);

// Utility macros for managing temporary allocations

#define ARENA_TEMP_BEGIN(arena) ArenaTempState ARENA_TEMP_END_NOT_CALLED = arenaSave(arena)
#define ARENA_TEMP_END(arena) arenaRestore(ARENA_TEMP_END_NOT_CALLED)
#define ARENA_TEMP_SCOPE(arena)                                \
    for (ArenaTempState CF_MACRO_VAR(temp) = arenaSave(arena); \
         CF_MACRO_VAR(temp).stack_id == arena->save_stack; arenaRestore(CF_MACRO_VAR(temp)))
