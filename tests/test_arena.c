
#include "foundation/memory.h"

#include "win32/win32.h"

#include <stdio.h>

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
    bool result = VirtualFree(memory, size, MEM_DECOMMIT);
    if (!result)
    {
        U32 err = GetLastError();
        CF_UNUSED(err);
        CF_ASSERT(false, "VM decommit failed");
    }
}

VM_RELEASE_FUNC(win32VmRelease)
{
    // NOTE (Matteo): VirtualFree(..., MEM_RELEASE) requires the base pointer
    // returned by VirtualFree(..., MEM_RESERVE) and a size of 0 to succeed.
    CF_UNUSED(size);

    bool result = VirtualFree(memory, 0, MEM_RELEASE);
    if (!result)
    {
        U32 err = GetLastError();
        CF_UNUSED(err);
        CF_ASSERT(false, "VM release failed");
    }
}

int
main()
{
    SYSTEM_INFO sysinfo;
    GetSystemInfo(&sysinfo);

    cfVirtualMemory vm = {
        .page_size = sysinfo.dwPageSize,
        .reserve = win32VmReserve,
        .commit = win32VmCommit,
        .revert = win32VmDecommit,
        .release = win32VmRelease,
    };

    Arena arena;

    if (!arenaInitOnVm(&arena, &vm, 1024 * 1024 * 1024))
    {
        printf("Cannot init memory arena");
        return -1;
    }

    I32 *ints = arenaAllocArray(&arena, I32, 1024);

    for (int i = 0; i < 1024; ++i)
    {
        ints[i] = i;
    }

    ARENA_TEMP_BEGIN(&arena);

    ints = arenaReallocArray(&arena, I32, ints, 1024, 2048);

    for (I32 i = 0; i < 1024; ++i)
    {
        CF_ASSERT(ints[i] == i, "");
    }

    ARENA_TEMP_END(&arena);

    for (I32 i = 1024; i < 2048; ++i)
    {
        ints[i] = i;
    }

    arenaClear(&arena);

    for (I32 i = 0; i < 512; ++i)
    {
        CF_ASSERT(ints[i] == i, "");
    }

    arenaReleaseVm(&arena);

    return 0;
}
