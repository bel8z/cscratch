#pragma once

/// Foundation memory services
/// This is not an API header, include it in implementation files only

#include "core.h"

//----------------------------//
//   Basic memory utilities   //
//----------------------------//

#define memAlloc(a, size) memAllocAlign(a, size, CF_MAX_ALIGN)
#define memAllocAlign(a, size, align) (a).func((a).state, NULL, 0, (size), (align))

#define memReallocAlign(a, mem, old_size, new_size, align) \
    (a).func((a).state, (mem), (old_size), (new_size), (align))
#define memRealloc(a, mem, old_size, new_size) \
    memReallocAlign(a, mem, old_size, new_size, CF_MAX_ALIGN)

#define cfMemFree(a, mem, size) (a).func((a).state, (void *)(mem), (size), 0, 0)

CF_API void memClear(void *mem, Usize count);
CF_API void memCopy(void const *from, void *to, Usize count);
CF_API void memCopySafe(void const *from, Usize from_size, void *to, Usize to_size);
CF_API void memWrite(U8 *mem, U8 value, Usize count);
CF_API I32 memCompare(void const *left, void const *right, Usize count);
CF_API bool memMatch(void const *left, void const *right, Usize count);

inline U8 const *
memAlignForward(U8 const *address, Usize alignment)
{
    CF_ASSERT((alignment & (alignment - 1)) == 0, "Alignment is not a power of 2");
    // Same as (address % alignment) but faster as alignment is a power of 2
    Uptr modulo = (Uptr)address & (alignment - 1);
    // Move pointer forward if needed
    return modulo ? address + alignment - modulo : address;
}

#define cfEqual(a, b) (CF_SAME_TYPE(a, b), memMatch(&a, &b, sizeof(a)))

//------------------------//
//   Virtual memory API   //
//------------------------//

// NOTE (Matteo): The implementation of this API must be provided by the platform layer
// TODO (Matteo): Improve mirror buffer API (and naming)

#define VM_RESERVE_FUNC(name) void *name(Usize size)
#define VM_RELEASE_FUNC(name) void name(void *memory, Usize size)

#define VM_COMMIT_FUNC(name) bool name(void *memory, Usize size)
#define VM_REVERT_FUNC(name) void name(void *memory, Usize size)

#define VM_MIRROR_ALLOCATE(name) VmMirrorBuffer name(Usize size)
#define VM_MIRROR_FREE(name) void name(VmMirrorBuffer *buffer)

/// Buffer built upon two adjacent virtual memory blocks that map to the same physical memory.
/// The memory is thus "mirrored" between the two blocks, hence the name.
/// Access is safe in the range [0, 2 * size), where the memory in [0, size) is exactly the same as
/// in [size, 2 * size)
typedef struct VmMirrorBuffer
{
    // Size of the buffer (it may be greater than requested to match address granularity)
    Usize size;
    // Pointer to start of the block
    void *data;
    // OS specific handle
    void *os_handle;
} VmMirrorBuffer;

/// Virtual memory access API
typedef struct CfVirtualMemory
{
    // Reserve a block of virtual memory, without committing it (memory can't be accessed)
    VM_RESERVE_FUNC((*reserve));
    // Release a block of reserved virtual memory
    VM_RELEASE_FUNC((*release));
    // Commit a portion of reserved virtual memory
    VM_COMMIT_FUNC((*commit));
    // Decommit a portion of reserved virtual memory
    VM_REVERT_FUNC((*revert));

    // Allocate a "mirror buffer" (single block of physical memory mapped to two adjacent blocks of
    // virtual memory)
    VM_MIRROR_ALLOCATE((*mirrorAllocate));
    // Release a "mirror buffer"
    VM_MIRROR_FREE((*mirrorFree));

    Usize page_size;
    Usize address_granularity;
} CfVirtualMemory;

#define vmReserve(vm, size) (vm)->reserve(size)
#define vmRelease(vm, mem, size) (vm)->release(mem, size)
#define vmCommit(vm, mem, size) (vm)->commit(mem, size)
#define vmRevert(vm, mem, size) (vm)->revert(mem, size)

#define vmMirrorAllocate(vm, size) (vm)->mirrorAllocate(size)
#define vmMirrorFree(vm, buff) (vm)->mirrorFree(buff)

//------------------//
//   Memory arena   //
//------------------//

/// Linear arena allocator that can use either a fixed size buffer or virtual
/// memory as a backing store
typedef struct MemArena
{
    // TODO (Matteo): Use U64 explicitly for sizes?

    Usize reserved;      // Reserved block size in bytes
    Usize allocated;     // Allocated (used) bytes count
    Usize committed;     // VM only - committed size in bytes
    Usize save_stack;    // Stack of saved states, as a progressive state ID.
    U8 *memory;          // Pointer to the reserved block
    CfVirtualMemory *vm; // VM only - API for VM operations
} MemArena;

/// Arena state, used for recovery after temporary allocations
typedef struct MemArenaState
{
    MemArena *arena;
    Usize allocated;
    Usize stack_id;
} MemArenaState;

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
CF_API bool memArenaInitOnVm(MemArena *arena, CfVirtualMemory *vm, void *reserved_block,
                             Usize reserved_size);

/// Initialize the arena using a pre-allocated memory buffer
CF_API void memArenaInitOnBuffer(MemArena *arena, U8 *buffer, Usize buffer_size);

/// Allocate a block of virtual memory and initialize an arena directly in it
CF_API MemArena *memArenaBootstrapFromVm(CfVirtualMemory *vm, void *reserved_block,
                                         Usize reserved_size);

// TODO (Matteo): Bootstrap from buffer

/// Free all the memory allocated by the arena. In case of a virtual memory backing
/// store, the memory is decommitted (returned to the OS)
CF_API void memArenaClear(MemArena *arena);

inline Usize
memArenaRemaining(MemArena *arena)
{
    return arena->reserved - arena->allocated;
}

/// Allocate a block of the given size and alignment from the top of the arena stack
CF_API void *memArenaAllocAlign(MemArena *arena, Usize size, Usize alignment);

/// Allocate a block of the given size and default alignment from the top of the arena stack
inline void *
memArenaAlloc(MemArena *arena, Usize size)
{
    return memArenaAllocAlign(arena, size, CF_MAX_ALIGN);
}

/// Try reallocating a block of the given size and alignment at the top of the arena stack
/// If the block does not match the last allocation, a new block is allocated
CF_API void *memArenaReallocAlign(MemArena *arena, void *memory, Usize old_size, Usize new_size,
                                  Usize alignment);

/// Try reallocating a block of the given size and default alignment at the top of the arena stack
/// If the block does not match the last allocation, a new block is allocated
inline void *
memArenaRealloc(MemArena *arena, void *memory, Usize old_size, Usize new_size)
{
    return memArenaReallocAlign(arena, memory, old_size, new_size, CF_MAX_ALIGN);
}

/// Returns a block of the given size to the top of the arena stack; this is
/// effective only if the block matches with the last allocation
CF_API void memArenaFree(MemArena *arena, void *memory, Usize size);

/// Allocates a block which fits the given struct on top of the arena stack
#define memArenaAllocStruct(arena, Type) \
    (Type *)memArenaAllocAlign(arena, sizeof(Type), alignof(Type))

/// Try freeing a block which fits the given struct on top of the arena stack
#define memArenaFreeStruct(arena, Type, ptr) memArenaFree(arena, ptr, sizeof(Type))

/// Allocates a block which fits the given array on top of the arena stack
#define memArenaAllocArray(arena, Type, count) \
    (Type *)memArenaAllocAlign(arena, count * sizeof(Type), alignof(Type))

/// Allocates a block which fits the given array on top of the arena stack
#define memArenaReallocArray(arena, Type, array, old_count, new_count)                             \
    (Type *)memArenaReallocAlign(arena, array, old_count * sizeof(Type), new_count * sizeof(Type), \
                                 alignof(Type))

/// Try freeing a block which fits the given array on top of the arena stack
#define memArenaFreeArray(arena, Type, count, ptr) memArenaFree(arena, ptr, count * sizeof(Type))

/// Save the current state of the arena, in order to restore it after temporary allocations.
CF_API MemArenaState memArenaSave(MemArena *arena);
/// Restore a previously saved state of an arena.
CF_API void memArenaRestore(MemArenaState state);

/// Split the arena in two smaller ones, each one responsible of its own block.
/// The size given is the minimum size of the chunk to split off the source arena, and can be larger
/// to accomodate the page sizes for VM backed arenas.
CF_API bool memArenaSplit(MemArena *arena, MemArena *split, Usize size);

/// Build a generic allocator based on the given arena
CF_API MemAllocator memArenaAllocator(MemArena *arena);

// Utility macros for managing temporary allocations

#define MEM_ARENA_TEMP_BEGIN(arena) \
    MemArenaState MEM_ARENA_TEMP_END_NOT_CALLED = memArenaSave(arena)

#define MEM_ARENA_TEMP_END(arena) memArenaRestore(MEM_ARENA_TEMP_END_NOT_CALLED)

#define MEM_ARENA_TEMP_SCOPE(arena)                              \
    for (MemArenaState CF_MACRO_VAR(temp) = memArenaSave(arena); \
         CF_MACRO_VAR(temp).stack_id == arena->save_stack; memArenaRestore(CF_MACRO_VAR(temp)))
