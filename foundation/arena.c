#include "arena.h"

#include "common.h"
#include "util.h"
#include "vm.h"

static inline u32
round_up(u32 block_size, u32 page_size)
{
    CF_ASSERT((page_size & (page_size - 1)) == 0, "Page size is not a power of 2");
    return page_size * ((block_size + page_size - 1) / page_size);
}

bool
arenaInitVm(Arena *arena, cfVirtualMemory *vm, u32 reserved_size)
{
    CF_ASSERT_NOT_NULL(arena);

    u32 rounded = round_up(reserved_size, vm->page_size);
    u8 *buffer = vm->reserve(rounded);

    if (!buffer) return false;

    arena->vm = vm;
    arena->memory = buffer;
    arena->reserved = rounded;
    arena->allocated = 0;

    return true;
}

void
arenaInitBuffer(Arena *arena, u8 *buffer, u32 buffer_size)
{
    CF_ASSERT_NOT_NULL(arena);

    arena->vm = NULL;
    arena->memory = buffer;
    arena->reserved = buffer_size;
    arena->allocated = 0;
}

void
arenaShutdown(Arena *arena)
{
    CF_ASSERT_NOT_NULL(arena);
    if (arena->vm) cfVmRelease(arena->vm, arena->memory, arena->reserved);
    // Make the arena unusable
    *arena = (Arena){0};
}

void
arenaFreeAll(Arena *arena)
{
    CF_ASSERT_NOT_NULL(arena);
    if (arena->vm) cfVmRevert(arena->vm, arena->memory, arena->reserved);
    arena->allocated = 0;
}

static void
arenaCommitVm(Arena *arena)
{
    CF_ASSERT_NOT_NULL(arena);

    if (arena->vm)
    {
        u32 commit_size = round_up(arena->allocated, arena->vm->page_size);
        cfVmCommit(arena->vm, arena->memory, commit_size);
    }
}

void *
arenaAllocAlign(Arena *arena, u32 size, u32 align)
{
    CF_ASSERT_NOT_NULL(arena);
    CF_ASSERT((align & (align - 1)) == 0, "Alignment is not a power of 2");

    // Align base pointer forward
    uptr ptr = (uptr)(arena->memory + arena->allocated);
    // Same as (ptr % align) but faster as align is a power of 2
    uptr modulo = ptr & (align - 1);
    uptr offset = ptr + align - modulo - (uptr)arena->memory;

    if (arena->reserved - offset < size) return NULL;

    u8 *result = arena->memory + offset;

    arena->allocated = offset + size;

    arenaCommitVm(arena);

    // NOTE (Matteo): For simplicity every allocation is cleared, even it can be
    // avoided for freshly committed VM pages
    cfMemClear(result, size);

    return result;
}

void *
arenaReallocAlign(Arena *arena, void *memory, u32 old_size, u32 new_size, u32 align)
{
    CF_ASSERT_NOT_NULL(arena);
    CF_ASSERT((align & (align - 1)) == 0, "Alignment is not a power of 2");

    u8 *block = memory;
    u8 const *block_end = block + old_size;
    u8 const *alloc_end = arena->memory + arena->allocated;
    u8 const *arena_end = arena->memory + arena->reserved;

    u8 *result = NULL;

    if (!block)
    {
        // New allocation
        CF_ASSERT(old_size == 0, "Allocating new block but old size given");
        result = arenaAllocAlign(arena, new_size, align);
    }
    else if (arena->memory <= block && block <= arena_end)
    {
        // The given block is in bounds; if it matches the last allocation it
        // can be expanded, otherwise a new allocation is performed
        if (block_end == alloc_end && block_end - old_size + new_size < arena_end)
        {
            arena->allocated += new_size - old_size;
            result = block;
            if (new_size > old_size)
            {
                arenaCommitVm(arena);
                cfMemClear(result + old_size, new_size - old_size);
            }
        }
        else
        {
            result = arenaAllocAlign(arena, new_size, align);
            if (result) cfMemCopy(memory, result, cfMin(old_size, new_size));
        }
    }
    else
    {
        CF_ASSERT(false, "Block is out of arena bounds");
    }

    return result;
}

void
arenaFree(Arena *arena, void const *memory, u32 size)
{
    CF_ASSERT_NOT_NULL(arena);

    u8 const *block = memory;
    u8 const *block_end = block + size;
    u8 const *alloc_end = arena->memory + arena->allocated;
    u8 const *arena_end = arena->memory + arena->reserved;

    if (!block)
    {
        CF_ASSERT(size == 0, "Freeing null block but size given");
    }
    else if (arena->memory <= block && block <= arena_end)
    {
        // Memory management is LIFO, so only the last allocated block can be
        // released
        if (block_end == alloc_end) arena->allocated -= size;
    }
    else
    {
        CF_ASSERT(false, "Block is out of arena bounds");
    }
}
