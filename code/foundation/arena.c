#include "arena.h"

#include "common.h"
#include "util.h"
#include "vm.h"

bool
arenaInitVm(Arena *arena, cfVirtualMemory *vm, U32 reserved_size)
{
    CF_ASSERT_NOT_NULL(arena);

    U32 rounded = cfRoundUp(reserved_size, vm->page_size);
    U8 *buffer = vm->reserve(rounded);

    if (!buffer) return false;

    arena->vm = vm;
    arena->memory = buffer;
    arena->reserved = rounded;
    arena->allocated = 0;
    arena->committed = 0;
    arena->save_stack = 0;

    return true;
}

void
arenaInitBuffer(Arena *arena, U8 *buffer, U32 buffer_size)
{
    CF_ASSERT_NOT_NULL(arena);

    arena->vm = NULL;
    arena->memory = buffer;
    arena->reserved = buffer_size;
    arena->allocated = 0;
    arena->committed = 0;
    arena->save_stack = 0;
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

    if (arena->vm && arena->committed > arena->allocated)
    {
        U32 max_commit_size = arena->reserved - arena->allocated;
        U32 commit_size = cfMin(
            max_commit_size, cfRoundUp(arena->allocated - arena->committed, arena->vm->page_size));
        cfVmCommit(arena->vm, arena->memory + arena->committed, commit_size);
        arena->committed += commit_size;
    }
}

void *
arenaAllocAlign(Arena *arena, U32 size, U32 align)
{
    CF_ASSERT_NOT_NULL(arena);
    CF_ASSERT((align & (align - 1)) == 0, "Alignment is not a power of 2");

    // Align base pointer forward
    Uptr ptr = (Uptr)(arena->memory + arena->allocated);
    // Same as (ptr % align) but faster as align is a power of 2
    Uptr modulo = ptr & (align - 1);
    Uptr offset = ptr + align - modulo - (Uptr)arena->memory;

    if (arena->reserved - offset < size) return NULL;

    U8 *result = arena->memory + offset;

    arena->allocated = offset + size;

    arenaCommitVm(arena);

    // NOTE (Matteo): For simplicity every allocation is cleared, even it can be
    // avoided for freshly committed VM pages
    cfMemClear(result, size);

    return result;
}

void *
arenaReallocAlign(Arena *arena, void *memory, U32 old_size, U32 new_size, U32 align)
{
    CF_ASSERT_NOT_NULL(arena);
    CF_ASSERT((align & (align - 1)) == 0, "Alignment is not a power of 2");

    U8 *block = memory;
    U8 const *block_end = block + old_size;
    U8 const *alloc_end = arena->memory + arena->allocated;
    U8 const *arena_end = arena->memory + arena->reserved;

    U8 *result = NULL;

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
arenaFree(Arena *arena, void const *memory, U32 size)
{
    CF_ASSERT_NOT_NULL(arena);

    U8 const *block = memory;
    U8 const *block_end = block + size;
    U8 const *alloc_end = arena->memory + arena->allocated;
    U8 const *arena_end = arena->memory + arena->reserved;

    if (!block)
    {
        CF_ASSERT(size == 0, "Freeing null block but size given");
    }
    else if (arena->memory <= block && block <= arena_end)
    {
        // Memory management is LIFO, so only the last allocated block can be
        // released
        if (block_end == alloc_end)
        {
#if CF_MEMORY_PROTECTION
            // NOTE (Matteo): Decommit unused memory to trigger access violations
            if (arena->vm) cfVmRevert(arena->vm, arena->memory + arena->allocated, size);
#endif
            arena->allocated -= size;
        }
    }
    else
    {
        CF_ASSERT(false, "Block is out of arena bounds");
    }
}

ArenaTempState
arenaSave(Arena *arena)
{
    CF_ASSERT_NOT_NULL(arena);
    return (ArenaTempState){
        .arena = arena,
        .allocated = arena->allocated,
        .stack_id = ++arena->save_stack,
    };
}

void
arenaRestore(ArenaTempState state)
{
    Arena *arena = state.arena;

    CF_ASSERT_NOT_NULL(arena);
    CF_ASSERT(arena->save_stack == state.stack_id, "Restoring invalid state");
    CF_ASSERT(arena->allocated >= state.allocated, "Restoring invalid state");

#if CF_MEMORY_PROTECTION
    // NOTE (Matteo): Decommit unused memory to trigger access violations
    if (arena->vm)
    {
        cfVmRevert(arena->vm, arena->memory + arena->allocated, arena->allocated - state.allocated);
    }
#endif

    arena->allocated = state.allocated;
    arena->save_stack--;
}

bool
arenaSplit(Arena *arena, Arena *split, U32 size)
{
    CF_ASSERT(!split->memory, "split is expected to be 0-initialized");

    // NOTE(Matteo): This can overflow, thus the additional test below
    U32 new_reserved = arena->reserved - size;
    if (size > arena->reserved || new_reserved < arena->allocated)
    {
        return false;
    }

    split->vm = arena->vm;
    split->memory = arena->memory + new_reserved;
    split->reserved = size;
    split->allocated = 0;
    split->committed = 0;
    split->save_stack = 0;

    if (arena->vm && arena->committed > new_reserved)
    {
        split->committed = arena->committed - new_reserved;
        arena->committed = new_reserved;
    }

    arena->reserved = new_reserved;

    return true;
}

static CF_ALLOCATOR_FUNC(arenaAllocProc)
{
    Arena *arena = state;

    if (new_size)
    {
        CF_ASSERT(memory || !old_size, "Invalid allocation request");
        return arenaReallocAlign(arena, memory, old_size, new_size, align);
    }

    CF_ASSERT(memory && old_size, "Invalid allocation request");

    arenaFree(arena, memory, old_size);
    return NULL;
}

cfAllocator
arenaAllocator(Arena *arena)
{
    return (cfAllocator){
        .state = arena,
        .func = arenaAllocProc,
    };
}
