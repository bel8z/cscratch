#include <stdio.h>
#include <stdlib.h>

#include "foundation/array.h"

static CF_ALLOCATOR_FUNC(reallocate)
{
    (void)(state);
    (void)(old_size);

    if (new_size) return realloc(memory, new_size);

    free(memory);
    return NULL;
}

int32_t
main(int32_t argc, char **argv)
{
    (void)argc;
    (void)argv;

    cfArray(i32) array = NULL;
    cfAllocator *alloc = &(cfAllocator){.reallocate = reallocate};
    cf_array_init(array, alloc, .capacity = 10);

    cfArrayPush(array, 0);
    cfArrayPush(array, 1);
    cfArrayPush(array, 2);

    printf("{");
    for (usize i = 0; i < cfArraySize(array); ++i)
    {
        printf("%d, ", array[i]);
    }
    printf("}\n");

    printf("\n");

    printf("pop: %d\n", cfArrayPop(array));
    printf("pop: %d\n", cfArrayPop(array));
    printf("pop: %d\n", cfArrayPop(array));

    printf("\n");

    printf("{");
    for (usize i = 0; i < cfArraySize(array); ++i)
    {
        printf("%d, ", array[i]);
    }
    printf("}\n");

    cfArrayPush(array, 0);
    cfArrayPush(array, 1);
    cfArrayPush(array, 2);
    cfArrayPush(array, 3);
    cfArrayPush(array, 4);

    printf("\n");

    printf("{");
    for (usize i = 0; i < cfArraySize(array); ++i)
    {
        printf("%d, ", array[i]);
    }
    printf("}\n");

    cfArrayRemove(array, 1);

    printf("{");
    for (usize i = 0; i < cfArraySize(array); ++i)
    {
        printf("%d, ", array[i]);
    }
    printf("}\n");

    cfArraySwapRemove(array, 1);

    printf("{");
    for (usize i = 0; i < cfArraySize(array); ++i)
    {
        printf("%d, ", array[i]);
    }
    printf("}\n");

    cfArrayInsert(array, 8, 1);

    printf("{");
    for (usize i = 0; i < cfArraySize(array); ++i)
    {
        printf("%d, ", array[i]);
    }
    printf("}\n");

    cfArrayFree(array);

    return 0;
}
