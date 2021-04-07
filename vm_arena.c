#include "vm_arena.h"

#include "common.h"

#include <string.h>

//------------------------------------------------------------------------------
// VM access API
//------------------------------------------------------------------------------

// NOTE (Matteo): The nomenclature is heavily influenced by the Win32 VM Api,
// which is currently the only supported implementation

static u32 vm_page_size();

static void *vm_reserve(u32 size);
static void vm_release(void *mem, u32 size);

static void vm_commit(void *mem, u32 size);
static void vm_decommit(void *mem, u32 size);

//------------------------------------------------------------------------------
// VM arena implementation
//------------------------------------------------------------------------------

static inline u32
round_up(u32 block_size, u32 page_size)
{
    ASSERT((page_size & (page_size - 1)) == 0, "Page size is not a power of 2");
    return page_size * ((block_size + page_size - 1) / page_size);
}

bool
arena_init(VmArena *arena, u32 reserved_size)
{
    arena->page_size = vm_page_size();
    arena->reserved = round_up(reserved_size, arena->page_size);
    arena->committed = 0;
    arena->allocated = 0;
    arena->memory = vm_reserve(arena->reserved);

    if (!arena->memory)
    {
        arena->page_size = 0;
        arena->reserved = 0;
        arena->memory = NULL;
        return false;
    }

    return true;
}

void
arena_free(VmArena *arena)
{
    vm_release(arena->memory, arena->reserved);
    // Make the arena unusable
    *arena = (VmArena){0};
}

void
arena_shrink(VmArena *arena)
{
    if (arena->allocated < arena->committed)
    {
        vm_decommit(arena->memory + arena->allocated,
                    arena->committed - arena->allocated);
    }
}

void *
arena_push_size(VmArena *arena, u32 size)
{
    if (arena->reserved - arena->allocated < size) return NULL;

    u8 *result = arena->memory + arena->allocated;

    arena->allocated += size;

    if (arena->allocated > arena->committed)
    {
        u32 commit_size = round_up(arena->allocated, arena->page_size);
        vm_commit(arena->memory + arena->committed, commit_size);
        arena->committed += commit_size;
    }
    else
    {
        // VM is cleared to 0 by the OS; here we are reusing already committed
        // memory so it is our responsibility to clean up
        u32 dirty = arena->committed - arena->allocated;
        memset(result + size - dirty, 0, dirty);
    }

    return result;
}

void
arena_pop_size(VmArena *arena, u32 size, void const *memory)
{
    // Memory management is LIFO, so only the last allocated block can be
    // released

    if (arena->memory + arena->allocated == (u8 *)memory + size)
    {
        arena->allocated -= size;
    }
}

//------------------------------------------------------------------------------
// VM access implementation
//------------------------------------------------------------------------------

#if defined(_WIN32)

#pragma warning(push)
#pragma warning(disable : 5105)

#include <Windows.h>

static void *
vm_reserve(u32 size)
{
    return VirtualAlloc(NULL, size, MEM_RESERVE, PAGE_NOACCESS);
}

static void
vm_commit(void *mem, u32 size)
{
    void *committed =
        VirtualAlloc(mem, size, MEM_COMMIT, PAGE_EXECUTE_READWRITE);
    ASSERT(committed, "Memory not previously reserved");
}

static void
vm_decommit(void *mem, u32 size)
{
    VirtualFree(mem, size, MEM_DECOMMIT);
}

static void
vm_release(void *mem, u32 size)
{
    VirtualFree(mem, size, MEM_RELEASE);
}

static u32
vm_page_size()
{
    SYSTEM_INFO sysinfo;
    GetSystemInfo(&sysinfo);
    return sysinfo.dwPageSize;
}

#pragma warning(pop)

#else
#error "Virtual memory arena is supported only on Windows"
#endif // defined(_WIN32)

//------------------------------------------------------------------------------
// Test
//------------------------------------------------------------------------------

#include <stdio.h>

int
main()
{
    VmArena arena;

    if (!arena_init(&arena, 1024 * 1024 * 1024))
    {
        printf("Cannot init memory arena");
        return -1;
    }

    int *ints = arena_push_array(&arena, int, 1024);

    for (int i = 0; i < 1024; ++i)
    {
        ints[i] = i;
    }

    arena_pop_array(&arena, int, 1024, ints);
    arena_shrink(&arena);
    arena_free(&arena);

    for (int i = 0; i < 512; ++i)
    {
        assert(ints[i] == i);
    }

    printf("YEAH!");

    return 0;
}
