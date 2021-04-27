#include "platform.h"

#if !defined(_WIN32)
#error "Win32 platform not supported"
#endif // defined(_WIN32)

#pragma warning(push)
#pragma warning(disable : 5105)

#include <windows.h>

// -----------------------------------------------------------------------------
// Internal function declarations
// -----------------------------------------------------------------------------

// VM functions
static VM_RESERVE_FUNC(win32VmReserve);
static VM_COMMIT_FUNC(win32VmCommit);
static VM_REVERT_FUNC(win32VmDecommit);
static VM_RELEASE_FUNC(win32VmRelease);

// Heap allocation functions
static CF_ALLOCATOR_FUNC(win32Alloc);
static CF_ALLOC_STATS_FUNC(win32AllocStats);

// -----------------------------------------------------------------------------
// Main API
// -----------------------------------------------------------------------------

cfPlatform
cfPlatformCreate()
{
    SYSTEM_INFO sysinfo;
    GetSystemInfo(&sysinfo);

    cfAllocatorStats *heap_stats =
        HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*heap_stats));

    cfPlatform plat = {
        .vm = {.reserve = win32VmReserve,
               .release = win32VmRelease,
               .commit = win32VmCommit,
               .revert = win32VmDecommit,
               .page_size = sysinfo.dwPageSize},
        .heap = {.state = heap_stats, .reallocate = win32Alloc, .stats = win32AllocStats}};

    return plat;
}

void
cfPlatformShutdown(cfPlatform *platform)
{
    CF_ASSERT_NOT_NULL(platform);

    cfAllocatorStats *heap_stats = platform->heap.state;

    CF_ASSERT_NOT_NULL(heap_stats);
    CF_ASSERT(heap_stats->count == 0, "Potential memory leak");

    // TODO (Matteo): Check allocation size tracking
    CF_ASSERT(heap_stats->size == 0, "Potential memory leak");

    HeapFree(GetProcessHeap(), 0, heap_stats);
}

// -----------------------------------------------------------------------------
// Internal functions
// -----------------------------------------------------------------------------

VM_RESERVE_FUNC(win32VmReserve)
{
    return VirtualAlloc(NULL, size, MEM_RESERVE, PAGE_NOACCESS);
}

VM_COMMIT_FUNC(win32VmCommit)
{
    void *committed = VirtualAlloc(memory, size, MEM_COMMIT, PAGE_READWRITE);
    CF_ASSERT(committed, "Memory not previously reserved");
    return committed != NULL;
}

VM_REVERT_FUNC(win32VmDecommit)
{
    VirtualFree(memory, size, MEM_DECOMMIT);
}

VM_RELEASE_FUNC(win32VmRelease)
{
    VirtualFree(memory, size, MEM_RELEASE);
}

CF_ALLOCATOR_FUNC(win32Alloc)
{
    // TODO (Matteo): New memory should be cleared by default?

    cfAllocatorStats *stats = state;

    if (new_size)
    {
        void *new_memory = NULL;

        if (memory)
        {
            CF_ASSERT(old_size > 0, "Reallocating memory but old size not given");
            new_memory = HeapReAlloc(GetProcessHeap(), 0, memory, new_size);
        }
        else
        {
            CF_ASSERT(old_size == 0, "Allocating new memory but old size given");
            new_memory = HeapAlloc(GetProcessHeap(), 0, new_size);
        }

        if (new_memory)
        {
            stats->size += new_size - old_size;
            stats->count += (old_size == 0);
        }

        return new_memory;
    }

    if (memory)
    {
        CF_ASSERT(old_size > 0, "Freeing memory but size not given");

        stats->count -= 1;
        stats->size -= old_size;
        HeapFree(GetProcessHeap(), 0, memory);
    }

    return NULL;
}

CF_ALLOC_STATS_FUNC(win32AllocStats)
{
    cfAllocatorStats *stats = state;
    return *stats;
}

#pragma warning(pop)
