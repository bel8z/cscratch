
#include "api.h"

#include "foundation/memory.h"

#include <stdio.h>

I32
platformMain(Platform *platform, Cstr argv[], I32 argc)
{
    CF_UNUSED(argc);
    CF_UNUSED(argv);

    MemArena arena;
    Usize const storage_size = 1024 * 1024 * 1024;
    void *storage = vmReserve(platform->vm, storage_size);

    if (!memArenaInitOnVm(&arena, platform->vm, storage, storage_size))
    {
        printf("Cannot init memory arena");
        return -1;
    }

    I32 *ints = memArenaAllocArray(&arena, I32, 1024);

    for (int i = 0; i < 1024; ++i)
    {
        ints[i] = i;
    }

    MEM_ARENA_TEMP_BEGIN(&arena);

    ints = memArenaReallocArray(&arena, ints, 1024, 2048);

    for (I32 i = 0; i < 1024; ++i)
    {
        CF_ASSERT(ints[i] == i, "");
    }

    MEM_ARENA_TEMP_END(&arena);

    for (I32 i = 1024; i < 2048; ++i)
    {
        ints[i] = i;
    }

    memArenaClear(&arena);

    for (I32 i = 0; i < 512; ++i)
    {
        CF_ASSERT(ints[i] == i, "");
    }

    vmRelease(platform->vm, storage, storage_size);

    return 0;
}
