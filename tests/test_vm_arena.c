
#include "foundation/platform.h"
#include "foundation/vm_arena.h"

#include <stdio.h>

int
main()
{
    cfPlatform p = cf_platform_create();

    VmArena arena;

    if (!arenaInit(&arena, &p.vm, 1024 * 1024 * 1024))
    {
        printf("Cannot init memory arena");
        return -1;
    }

    int *ints = arenaPushArray(&arena, int, 1024);

    for (int i = 0; i < 1024; ++i)
    {
        ints[i] = i;
    }

    arenaPopArray(&arena, int, 1024, ints);
    arenaShrink(&arena);
    arenaFree(&arena);

    for (int i = 0; i < 512; ++i)
    {
        assert(ints[i] == i);
    }

    printf("YEAH!");

    return 0;
}
