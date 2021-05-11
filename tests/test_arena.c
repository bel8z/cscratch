
#include "foundation/arena.h"
#include "foundation/platform.h"

#include <stdio.h>

int
main()
{
    cfPlatform p = cfPlatformCreate();

    Arena arena;

    if (!arenaInitVm(&arena, &p.vm, 1024 * 1024 * 1024))
    {
        printf("Cannot init memory arena");
        return -1;
    }

    i32 *ints = arenaAllocArray(&arena, i32, 1024);

    for (int i = 0; i < 1024; ++i)
    {
        ints[i] = i;
    }

    ARENA_TEMP_BEGIN(&arena);

    ints = arenaReallocArray(&arena, i32, ints, 1024, 2048);

    for (i32 i = 0; i < 1024; ++i)
    {
        assert(ints[i] == i);
    }

    ARENA_TEMP_END(&arena);

    for (i32 i = 1024; i < 2048; ++i)
    {
        ints[i] = i;
    }

    arenaFreeAll(&arena);

    for (i32 i = 0; i < 512; ++i)
    {
        assert(ints[i] == i);
    }

    arenaShutdown(&arena);

    return 0;
}
