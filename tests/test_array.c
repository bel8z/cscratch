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

    int32_t *array = NULL;
    CfAllocator *alloc = &(CfAllocator){.reallocate = reallocate};
    cf_array_init(array, alloc, .capacity = 10);

    cf_array_push(array, 0);
    cf_array_push(array, 1);
    cf_array_push(array, 2);

    printf("{");
    for (size_t i = 0; i < cf_array_size(array); ++i)
    {
        printf("%d, ", array[i]);
    }
    printf("}\n");

    printf("\n");

    printf("pop: %d\n", cf_array_pop(array));
    printf("pop: %d\n", cf_array_pop(array));
    printf("pop: %d\n", cf_array_pop(array));

    printf("\n");

    printf("{");
    for (size_t i = 0; i < cf_array_size(array); ++i)
    {
        printf("%d, ", array[i]);
    }
    printf("}\n");

    cf_array_push(array, 0);
    cf_array_push(array, 1);
    cf_array_push(array, 2);
    cf_array_push(array, 3);
    cf_array_push(array, 4);

    printf("\n");

    printf("{");
    for (size_t i = 0; i < cf_array_size(array); ++i)
    {
        printf("%d, ", array[i]);
    }
    printf("}\n");

    cf_array_remove(array, 1);

    printf("{");
    for (size_t i = 0; i < cf_array_size(array); ++i)
    {
        printf("%d, ", array[i]);
    }
    printf("}\n");

    cf_array_swap_remove(array, 1);

    printf("{");
    for (size_t i = 0; i < cf_array_size(array); ++i)
    {
        printf("%d, ", array[i]);
    }
    printf("}\n");

    cf_array_insert(array, 8, 1);

    printf("{");
    for (size_t i = 0; i < cf_array_size(array); ++i)
    {
        printf("%d, ", array[i]);
    }
    printf("}\n");

    cf_array_free(array);

    return 0;
}
