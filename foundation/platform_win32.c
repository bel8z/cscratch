#include "platform.h"

#if !defined(_WIN32)
#error "Win32 platform not supported"
#endif // defined(_WIN32)

#pragma warning(push)
#pragma warning(disable : 5105)

#include <Windows.h>

// -----------------------------------------------------------------------------
// Internal function declarations
// -----------------------------------------------------------------------------

// VM functions
static VM_RESERVE_FUNC(win32_vm_reserve);
static VM_COMMIT_FUNC(win32_vm_commit);
static VM_REVERT_FUNC(win32_vm_decommit);
static VM_RELEASE_FUNC(win32_vm_release);

// Heap allocation functions
static CF_ALLOCATOR_FUNC(win32_alloc_func);
static CF_ALLOC_STATS_FUNC(win32_alloc_stats);

// -----------------------------------------------------------------------------
// Internal globals
// -----------------------------------------------------------------------------

static CfAllocatorStats win32_heap_alloc = {0};

// -----------------------------------------------------------------------------
// Main API
// -----------------------------------------------------------------------------

Platform
platform_create()
{
    SYSTEM_INFO sysinfo;
    GetSystemInfo(&sysinfo);

    Platform plat = {.vm = {.reserve = win32_vm_reserve,
                            .release = win32_vm_release,
                            .commit = win32_vm_commit,
                            .revert = win32_vm_decommit,
                            .page_size = sysinfo.dwPageSize},
                     .heap = {.state = &win32_heap_alloc,
                              .reallocate = win32_alloc_func,
                              .stats = win32_alloc_stats}};

    return plat;
}

// -----------------------------------------------------------------------------
// Internal functions
// -----------------------------------------------------------------------------

VM_RESERVE_FUNC(win32_vm_reserve)
{
    CF_UNUSED(state);
    return VirtualAlloc(NULL, size, MEM_RESERVE, PAGE_NOACCESS);
}

VM_COMMIT_FUNC(win32_vm_commit)
{
    CF_UNUSED(state);
    void *committed = VirtualAlloc(memory, size, MEM_COMMIT, PAGE_EXECUTE_READWRITE);
    CF_ASSERT(committed, "Memory not previously reserved");
    return committed != NULL;
}

VM_REVERT_FUNC(win32_vm_decommit)
{
    CF_UNUSED(state);
    VirtualFree(memory, size, MEM_DECOMMIT);
}

VM_RELEASE_FUNC(win32_vm_release)
{
    CF_UNUSED(state);
    VirtualFree(memory, size, MEM_RELEASE);
}

CF_ALLOCATOR_FUNC(win32_alloc_func)
{
    CfAllocatorStats *stats = state;

    if (new_size)
    {
        if (memory && old_size)
        {
            stats->size += new_size - old_size;
        }
        else
        {
            stats->count++;
            stats->size += new_size;
        }

        return HeapAlloc(GetProcessHeap(), 0, new_size);
    }

    if (memory)
    {
        stats->count -= 1;
        stats->size -= old_size;
        HeapFree(GetProcessHeap(), 0, memory);
    }

    return NULL;
}

static CF_ALLOC_STATS_FUNC(win32_alloc_stats)
{
    CfAllocatorStats *stats = state;
    return *stats;
}

#pragma warning(pop)
