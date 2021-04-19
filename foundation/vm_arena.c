#include "vm_arena.h"

#include "common.h"
#include "util.h"

static inline u32
round_up(u32 block_size, u32 page_size)
{
    CF_ASSERT((page_size & (page_size - 1)) == 0, "Page size is not a power of 2");
    return page_size * ((block_size + page_size - 1) / page_size);
}

bool
arenaInit(VmArena *arena, cfVirtualMemory *vm, u32 reserved_size)
{
    u32 rounded = round_up(reserved_size, vm->page_size);
    u8 *buffer = vm->reserve(rounded);

    if (!buffer) return false;

    arena->vm = vm;
    arena->memory = buffer;
    arena->reserved = rounded;
    arena->committed = 0;
    arena->allocated = 0;

    return true;
}

void
arenaFree(VmArena *arena)
{
    arena->vm->release(arena->memory, arena->reserved);
    // Make the arena unusable
    *arena = (VmArena){0};
}

void
arenaShrink(VmArena *arena)
{
    if (arena->allocated < arena->committed)
    {
        arena->vm->revert(arena->memory + arena->allocated, arena->committed - arena->allocated);
    }
}

void *
arenaPushSize(VmArena *arena, u32 size)
{
    if (arena->reserved - arena->allocated < size) return NULL;

    u8 *result = arena->memory + arena->allocated;

    arena->allocated += size;

    if (arena->allocated > arena->committed)
    {
        u32 commit_size = round_up(arena->allocated, arena->vm->page_size);
        arena->vm->commit(arena->memory + arena->committed, commit_size);
        arena->committed += commit_size;
    }
    else
    {
        // VM is cleared to 0 by the OS; here we are reusing already committed
        // memory so it is our responsibility to clean up
        u32 dirty = arena->committed - arena->allocated;
        cfMemWrite(result + size - dirty, 0, dirty);
    }

    return result;
}

void
arenaPopSize(VmArena *arena, u32 size, void const *memory)
{
    // Memory management is LIFO, so only the last allocated block can be
    // released

    if (arena->memory + arena->allocated == (u8 *)memory + size)
    {
        arena->allocated -= size;
    }
}
