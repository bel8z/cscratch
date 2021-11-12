#include "memory.h"
#include "core.h"
#include "error.h"
#include "util.h"

#include <string.h>

//----------------------------//
//   Basic memory utilities   //
//----------------------------//

void
memClear(void *mem, Usize count)
{
    memset(mem, 0, count); // NOLINT
}

void
memCopy(void const *from, void *to, Usize count)
{
    memmove(to, from, count); // NOLINT
}

void
memCopySafe(void const *from, Usize from_size, void *to, Usize to_size)
{
    memmove_s(to, to_size, from, from_size);
}

void
memWrite(U8 *mem, U8 value, Usize count)
{
    memset(mem, value, count); // NOLINT
}

I32
memCompare(void const *left, void const *right, Usize count)
{
    return memcmp(left, right, count);
}

bool
memMatch(void const *left, void const *right, Usize count)
{
    return !memcmp(left, right, count);
}

U8 const *
memAlignForward(U8 const *address, Usize alignment)
{
    CF_ASSERT((alignment & (alignment - 1)) == 0, "Alignment is not a power of 2");
    // Same as (address % alignment) but faster as alignment is a power of 2
    Uptr modulo = (Uptr)address & (alignment - 1);
    // Move pointer forward if needed
    return modulo ? address + alignment - modulo : address;
}

//---------------------------//
//   End-of-page allocator   //
//---------------------------//

static Usize
memRoundSize(Usize req_size, Usize page_size)
{
    Usize page_count = (req_size + page_size - 1) / page_size;
    return page_count * page_size;
}

static MEM_ALLOCATOR_FUNC(memEndOfPageAlloc)
{
    CfVirtualMemory *vm = state;

    // NOTE (Matteo): This function cannot guarantee the required alignment
    // because of the stricter requirement to align the block at the end of
    // a page; as such, UBSan can complain about misaligned accessess and
    // SIMD data structures can pose a problem if not allocated in power-of-2 sizes.

    CF_ASSERT((align & (align - 1)) == 0, "Alignment is not a power of 2");

    void *new_mem = NULL;

    if (new_size)
    {
        Usize block_size = memRoundSize(new_size, vm->page_size);
        U8 *base = vmReserve(vm, block_size);
        if (!base) return NULL;

        vmCommit(vm, base, block_size);

        new_mem = base + block_size - new_size;
    }

    if (memory)
    {
        CF_ASSERT(old_size > 0, "Freeing valid pointer but given size is 0");

        if (new_mem)
        {
            memCopy(memory, new_mem, cfMin(old_size, new_size));
        }

        Usize block_size = memRoundSize(old_size, vm->page_size);
        U8 *base = (U8 *)memory + old_size - block_size;

        vmRevert(vm, base, block_size);
        vmRelease(vm, base, block_size);

        memory = NULL;
    }

    return new_mem;
}

MemAllocator
memEndOfPageAllocator(CfVirtualMemory *vm)
{
    return (MemAllocator){.func = memEndOfPageAlloc, .state = vm};
}

//------------------//
//   Memory arena   //
//------------------//

static void
arenaCommitVm(MemArena *arena)
{
    CF_ASSERT_NOT_NULL(arena);

    if (arena->vm && arena->allocated > arena->committed)
    {
        // NOTE (Matteo): Align memory commits to page boundaries
        U8 const *next_pos =
            memAlignForward(arena->memory + arena->allocated, arena->vm->page_size);
        U8 *curr_pos = arena->memory + arena->committed;

        Usize max_commit_size = arena->reserved - arena->allocated;
        Usize commit_size = cfMin(next_pos - curr_pos, max_commit_size);

        vmCommit(arena->vm, curr_pos, commit_size);
        arena->committed += commit_size;
    }
}

static void
arenaDecommitVm(MemArena *arena)
{
    if (arena->vm && arena->committed > arena->allocated)
    {
        // NOTE (Matteo): Since VM decommit acts on full pages, I need to align the block to be
        // decommitted 1 page up in order to preserve the last page which is partially filled

        // Align base pointer forward
        U8 const *base = arena->memory + arena->allocated;
        Usize offset = memAlignForward(base, arena->vm->page_size) - arena->memory;

        if (offset < arena->committed)
        {
            Usize decommit_size = arena->committed - offset;
            vmRevert(arena->vm, arena->memory + offset, decommit_size);
            arena->committed -= decommit_size;
        }
    }
}

bool
memArenaInitOnVm(MemArena *arena, CfVirtualMemory *vm, void *reserved_block, Usize reserved_size)
{
    CF_ASSERT_NOT_NULL(arena);

    arena->vm = vm;
    arena->memory = reserved_block;
    arena->reserved = reserved_size;
    arena->allocated = 0;
    arena->committed = 0;
    arena->save_stack = 0;

    return true;
}

void
memArenaInitOnBuffer(MemArena *arena, U8 *buffer, Usize buffer_size)
{
    CF_ASSERT_NOT_NULL(arena);

    arena->vm = NULL;
    arena->memory = buffer;
    arena->reserved = buffer_size;
    arena->allocated = 0;
    arena->committed = 0;
    arena->save_stack = 0;
}

MemArena *
memArenaBootstrapFromVm(CfVirtualMemory *vm, void *reserved_block, Usize reserved_size)
{
    MemArena *arena = NULL;

    if (reserved_block)
    {
        CF_ASSERT(reserved_size > sizeof(*arena), "Cannot bootstrap arena from smaller allocation");

        Usize commit_size = cfMin(reserved_size, cfRoundUp(sizeof(*arena), vm->page_size));
        vmCommit(vm, reserved_block, commit_size);

        arena = reserved_block;
        arena->vm = vm;
        arena->memory = (U8 *)arena;
        arena->reserved = reserved_size;
        arena->allocated = sizeof(*arena);
        arena->committed = commit_size;
        arena->save_stack = 0;
    }

    return arena;
}

MemArena *
memArenaBootstrapFromBuffer(U8 *buffer, Usize buffer_size)
{
    MemArena *arena = NULL;

    if (buffer)
    {
        CF_ASSERT(buffer_size > sizeof(*arena), "Cannot bootstrap arena from smaller allocation");

        arena = (MemArena *)buffer;
        arena->vm = NULL;
        arena->memory = buffer;
        arena->reserved = buffer_size;
        arena->allocated = sizeof(*arena);
        arena->committed = 0;
        arena->save_stack = 0;
    }

    return arena;
}

void
memArenaClear(MemArena *arena)
{
    CF_ASSERT_NOT_NULL(arena);

    if (arena->memory == (U8 *)arena)
    {
        // Arena is bootstrapped, so I need to preserve its allocation
        arena->allocated = sizeof(*arena);
    }
    else
    {
        arena->allocated = 0;
    }

    arenaDecommitVm(arena);
}

void *
memArenaAllocAlign(MemArena *arena, Usize size, Usize align)
{
    CF_ASSERT_NOT_NULL(arena);
    CF_ASSERT((align & (align - 1)) == 0, "Alignment is not a power of 2");

    U8 *result = NULL;

    // Align base pointer forward
    U8 const *base = arena->memory + arena->allocated;
    Usize offset = memAlignForward(base, align) - arena->memory;

    if (offset + size <= arena->reserved)
    {
        result = arena->memory + offset;
        arena->allocated = offset + size;

        arenaCommitVm(arena);

        // NOTE (Matteo): For simplicity every allocation is cleared, even it can be
        // avoided for freshly committed VM pages
        memClear(result, size);
    }

    return result;
}

void *
memArenaReallocAlign(MemArena *arena, void *memory, Usize old_size, Usize new_size, Usize align)
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
        result = memArenaAllocAlign(arena, new_size, align);
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
                memClear(result + old_size, new_size - old_size);
            }
        }
        else
        {
            result = memArenaAllocAlign(arena, new_size, align);
            if (result) memCopy(memory, result, cfMin(old_size, new_size));
        }
    }
    else
    {
        CF_ASSERT(false, "Block is out of arena bounds");
    }

    return result;
}

void
memArenaFree(MemArena *arena, void *memory, Usize size)
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
        // Memory management is LIFO, so only the last allocated block can be freed
        if (block_end == alloc_end)
        {
            arena->allocated -= size;
#if CF_MEMORY_PROTECTION
            // NOTE (Matteo): Decommit unused memory to trigger access violations
            arenaDecommitVm(arena);
#endif
        }
    }
    else
    {
        CF_ASSERT(false, "Block is out of arena bounds");
    }
}

MemArenaState
memArenaSave(MemArena *arena)
{
    CF_ASSERT_NOT_NULL(arena);
    return (MemArenaState){
        .arena = arena,
        .allocated = arena->allocated,
        .stack_id = ++arena->save_stack,
    };
}

void
memArenaRestore(MemArenaState state)
{
    MemArena *arena = state.arena;

    CF_ASSERT_NOT_NULL(arena);
    CF_ASSERT(arena->save_stack == state.stack_id, "Restoring invalid state");
    CF_ASSERT(arena->allocated >= state.allocated, "Restoring invalid state");

    arena->allocated = state.allocated;
    arena->save_stack--;

#if CF_MEMORY_PROTECTION
    // NOTE (Matteo): Decommit unused memory to trigger access violations
    arenaDecommitVm(arena);
#endif
}

bool
memArenaSplit(MemArena *arena, MemArena *split, Usize size)
{
    CF_ASSERT(!split->memory, "split is expected to be 0-initialized");

    if (size > arena->reserved) return false;

    Usize new_reserved = arena->reserved - size;

    if (arena->vm)
    {
        // NOTE (Matteo): When VM is involved, split must occur on page boundaries
        Uptr new_end = (Uptr)(arena->memory + new_reserved);
        Uptr modulo = new_end & (arena->vm->page_size - 1);
        new_reserved -= modulo;
    }

    if (new_reserved < arena->allocated) return false;

    split->vm = arena->vm;
    split->memory = arena->memory + new_reserved;
    split->reserved = arena->reserved - new_reserved;
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

static MEM_ALLOCATOR_FUNC(arenaAllocProc)
{
    CF_ASSERT(memory || !old_size, "Invalid allocation request");

    MemArena *arena = state;
    void *new_memory = NULL;

    if (new_size)
    {
        new_memory = memArenaReallocAlign(arena, memory, old_size, new_size, align);
    }
    else
    {
        memArenaFree(arena, memory, old_size);
    }

    return new_memory;
}

MemAllocator
memArenaAllocator(MemArena *arena)
{
    return (MemAllocator){
        .state = arena,
        .func = arenaAllocProc,
    };
}
