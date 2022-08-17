#pragma once

/// Foundation memory services
/// This is not an API header, include it in implementation files only

#include "core.h"

//----------------------------//
//   Basic memory utilities   //
//----------------------------//

//=== Write/Copy ===//

CF_API void memClear(void *mem, Usize count);
CF_API void memWrite(U8 *mem, U8 value, Usize count);
CF_API void memCopy(void const *from, void *to, Usize count);
CF_API void memCopySafe(void const *from, Usize from_size, void *to, Usize to_size);

#define memClearStruct(ptr) memClear(ptr, sizeof(*(ptr)))
#define memClearArray(ptr, count) memClear(ptr, (count) * sizeof(*(ptr)))

#define memCopyArray(from, to, count) \
    (CF_SAME_TYPE(*(from), *(to)), memCopy(from, to, (count) * sizeof(*(from))))

//=== Comparison ===//

CF_API I32 memCompare(void const *left, void const *right, Usize count);
CF_API bool memMatch(void const *left, void const *right, Usize count);

#define memMatchStruct(a, b) (CF_SAME_TYPE(a, b), memMatch(&a, &b, sizeof(a)))
#define memMatchArray(a, b, count) \
    (CF_SAME_TYPE(*(a), *(b)), memMatch(a, b, (count) * sizeof(*(a))))

//=== Alignment ===//

U8 const *memAlignForward(U8 const *address, Usize alignment);

//=== Allocation ===//

CF_INLINE_API void *memAlloc(MemAllocator a, Usize size);
CF_INLINE_API void *memAllocAlign(MemAllocator a, Usize size, Usize align);

CF_INLINE_API void *memRealloc(MemAllocator a, void *mem, Usize old_size, Usize new_size);
CF_INLINE_API void *memReallocAlign(MemAllocator a, void *mem, Usize old_size, Usize new_size,
                                    Usize align);

CF_INLINE_API void memFree(MemAllocator a, void *mem, Usize size);
CF_INLINE_API void memFreeAlign(MemAllocator a, void *mem, Usize size, Usize align);

// NOTE (Matteo): Those are the very basics required to implement a dynamic array
// and are offered in case the full-fledged MemBuffer is not needed or suitable.
#if CF_COMPILER_CLANG
#    define memAllocStruct(allocator, Type) memAllocAlign(allocator, sizeof(Type), alignof(Type))
#    define memFreeStruct(allocator, ptr) \
        memFreeAlign(allocator, ptr, sizeof(*(ptr)), alignof(*(ptr)))

#    define memAllocArray(allocator, Type, cap) \
        memReallocAlign(allocator, NULL, 0, (cap) * sizeof(Type), alignof(Type))

#    define memReallocArray(allocator, ptr, old_cap, new_cap)                                   \
        memReallocAlign(allocator, ptr, (old_cap) * sizeof(*(ptr)), (new_cap) * sizeof(*(ptr)), \
                        alignof(*(ptr)))

#    define memFreeArray(allocator, ptr, cap) \
        memFreeAlign(allocator, ptr, (cap) * sizeof(*(ptr)), alignof(*(ptr)))
#else
#    define memAllocStruct(allocator, Type) memAlloc(allocator, sizeof(Type))
#    define memFreeStruct(allocator, ptr) memFree(allocator, ptr, sizeof(*(ptr)))

#    define memAllocArray(allocator, Type, cap) memRealloc(allocator, NULL, 0, (cap) * sizeof(Type))

#    define memReallocArray(allocator, ptr, old_cap, new_cap) \
        memRealloc(allocator, ptr, (old_cap) * sizeof(*(ptr)), (new_cap) * sizeof(*(ptr)))

#    define memFreeArray(allocator, ptr, cap) memFree(allocator, ptr, (cap) * sizeof(*(ptr)))
#endif

//------------------------//
//   Virtual memory API   //
//------------------------//

// NOTE (Matteo): The implementation of this API must be provided by the platform layer
// TODO (Matteo): Improve mirror buffer API (and naming)

#define VMEM_RESERVE_FN(name) void *name(Usize size)
#define VMEM_RELEASE_FN(name) void name(void *memory, Usize size)

#define VMEM_COMMIT_FN(name) bool name(void *memory, Usize size)
#define VMEM_DECOMMIT_FN(name) void name(void *memory, Usize size)

#define VMEM_MIRROR_ALLOCATE_FN(name) VMemMirrorBuffer name(Usize size)
#define VMEM_MIRROR_FREE_FN(name) void name(VMemMirrorBuffer *buffer)

/// Buffer built upon two adjacent virtual memory blocks that map to the same physical memory.
/// The memory is thus "mirrored" between the two blocks, hence the name.
/// Access is safe in the range [0, 2 * size), where the memory in [0, size) is exactly the same as
/// in [size, 2 * size)
typedef struct VMemMirrorBuffer
{
    // Size of the buffer (it may be greater than requested to match address granularity)
    Usize size;
    // Pointer to start of the block
    void *data;
    // OS specific handle
    void *os_handle;
} VMemMirrorBuffer;

/// Virtual memory access API
typedef struct VMemApi
{
    // Reserve a block of virtual memory, without committing it (memory can't be accessed)
    VMEM_RESERVE_FN((*reserve));
    // Release a block of reserved virtual memory
    VMEM_RELEASE_FN((*release));
    // Commit a portion of reserved virtual memory
    VMEM_COMMIT_FN((*commit));
    // Decommit a portion of reserved virtual memory
    VMEM_DECOMMIT_FN((*decommit));

    // Allocate a "mirror buffer" (single block of physical memory mapped to two adjacent blocks of
    // virtual memory)
    VMEM_MIRROR_ALLOCATE_FN((*mirrorAllocate));
    // Release a "mirror buffer"
    VMEM_MIRROR_FREE_FN((*mirrorFree));

    Usize page_size;
    Usize address_granularity;
} VMemApi;

#define vmemReserve(vmem, size) (vmem)->reserve(size)
#define vmemRelease(vmem, mem, size) (vmem)->release(mem, size)
#define vmemCommit(vmem, mem, size) (vmem)->commit(mem, size)
#define vmemDecommit(vmem, mem, size) (vmem)->decommit(mem, size)

#define vmemMirrorAllocate(vmem, size) (vmem)->mirrorAllocate(size)
#define vmemMirrorFree(vmem, buff) (vmem)->mirrorFree(buff)

//---------------------------//
//   End-of-page allocator   //
//---------------------------//

// NOTE (Matteo): This allocator is useful for out-of-bounds access since it allocates
// blocks at the end of virtual memory pages.

MemAllocator memEndOfPageAllocator(VMemApi *vmem);

//------------------//
//   Memory arena   //
//------------------//

/// Linear arena allocator that can use either a fixed size buffer or virtual
/// memory as a backing store
typedef struct MemArena
{
    // TODO (Matteo): Use U64 explicitly for sizes?

    Usize reserved;   // Reserved block size in bytes
    Usize allocated;  // Allocated (used) bytes count
    Usize committed;  // VMem only - committed virtual memory in bytes
    Usize save_stack; // Stack of saved states, as a progressive state ID.
    U8 *memory;       // Pointer to the reserved block
    VMemApi *vmem;    // VMem only - API for virtual memory operations
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
CF_API void memArenaInitOnVmem(MemArena *arena, VMemApi *vmem, void *reserved_block,
                               Usize reserved_size);

/// Initialize the arena using a pre-allocated memory buffer
CF_API void memArenaInitOnBuffer(MemArena *arena, U8 *buffer, Usize buffer_size);

/// Allocate a block of virtual memory and initialize an arena directly in it
CF_API MemArena *memArenaBootstrapFromVmem(VMemApi *vmem, void *reserved_block,
                                           Usize reserved_size);

// TODO (Matteo): Bootstrap from buffer
CF_API MemArena *memArenaBootstrapFromBuffer(U8 *buffer, Usize buffer_size);

/// Free all the memory allocated by the arena. In case of a virtual memory backing
/// store, the memory is decommitted (returned to the OS)
CF_API void memArenaClear(MemArena *arena);

/// Return the amount of remaining memory available for allocation
CF_INLINE_API Usize memArenaAvailable(MemArena *arena);

/// Allocate a block of the given size and default alignment from the top of the arena stack
CF_INLINE_API void *memArenaAlloc(MemArena *arena, Usize size);

/// Allocate a block of the given size and alignment from the top of the arena stack
CF_API void *memArenaAllocAlign(MemArena *arena, Usize size, Usize alignment);

/// Try reallocating a block of the given size and default alignment at the top of the arena stack
/// If the block does not match the last allocation, a new block is allocated
CF_INLINE_API void *memArenaRealloc(MemArena *arena, void *memory, Usize old_size, Usize new_size);

/// Try reallocating a block of the given size and alignment at the top of the arena stack
/// If the block does not match the last allocation, a new block is allocated
CF_API void *memArenaReallocAlign(MemArena *arena, void *memory, Usize old_size, Usize new_size,
                                  Usize alignment);

/// Returns a block of the given size to the top of the arena stack; this is
/// effective only if the block matches with the last allocation
CF_API void memArenaFree(MemArena *arena, void *memory, Usize size);

/// Allocates a block which fits the given struct on top of the arena stack
#define memArenaAllocStruct(arena, Type) memArenaAllocAlign(arena, sizeof(Type), alignof(Type))

/// Try freeing a block which fits the given struct on top of the arena stack
#define memArenaFreeStruct(arena, Type, ptr) memArenaFree(arena, ptr, sizeof(Type))

#if CF_COMPILER_CLANG
/// Allocates a block which fits the given array on top of the arena stack
#    define memArenaAllocArray(arena, Type, count) \
        memArenaAllocAlign(arena, (count) * sizeof(Type), alignof(Type))

/// Allocates a block which fits the given array on top of the arena stack
#    define memArenaReallocArray(arena, ptr, old_count, new_count)     \
        memArenaReallocAlign(arena, ptr, (old_count) * sizeof(*(ptr)), \
                             (new_count) * sizeof(*(ptr)), alignof(*(ptr)))

#else
/// Allocates a block which fits the given array on top of the arena stack
#    define memArenaAllocArray(arena, Type, count) memArenaAlloc(arena, (count) * sizeof(Type))

/// Allocates a block which fits the given array on top of the arena stack
#    define memArenaReallocArray(arena, ptr, old_count, new_count) \
        memArenaRealloc(arena, ptr, (old_count) * sizeof(*(ptr)), (new_count) * sizeof(*(ptr)))
#endif

/// Try freeing a block which fits the given array on top of the arena stack
#define memArenaFreeArray(arena, ptr, count) memArenaFree(arena, ptr, (count) * sizeof(*(ptr)))

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

//----------------------------//
