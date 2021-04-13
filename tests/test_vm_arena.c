
#include "foundation/vm_arena.h"

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
