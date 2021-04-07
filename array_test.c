#include <stdio.h>
#include <stdlib.h>

#include "array.h"

static void *allocate(size_t size, void *state)
{
    (void)(state);
    return malloc(size);
}

static void *reallocate(void *memory, size_t size, void *state)
{
    (void)(state);
    return realloc(memory, size);
}

static void deallocate(void *memory, void *state)
{
    (void)(state);
    free(memory);
}

int32_t main(int32_t argc, char **argv)
{
    (void)argc;
    (void)argv;

    int32_t *array = NULL;
    Allocator *alloc = &(Allocator){.allocate = allocate,
                                    .deallocate = deallocate,
                                    .reallocate = reallocate};
    array_init(array, alloc, .capacity = 10);

    array_push(array, 0);
    array_push(array, 1);
    array_push(array, 2);

    printf("{");
    for (size_t i = 0; i < array_size(array); ++i)
    {
        printf("%d, ", array[i]);
    }
    printf("}\n");

    printf("\n");

    printf("pop: %d\n", array_pop(array));
    printf("pop: %d\n", array_pop(array));
    printf("pop: %d\n", array_pop(array));

    printf("\n");

    printf("{");
    for (size_t i = 0; i < array_size(array); ++i)
    {
        printf("%d, ", array[i]);
    }
    printf("}\n");

    array_push(array, 0);
    array_push(array, 1);
    array_push(array, 2);
    array_push(array, 3);
    array_push(array, 4);

    printf("\n");

    printf("{");
    for (size_t i = 0; i < array_size(array); ++i)
    {
        printf("%d, ", array[i]);
    }
    printf("}\n");

    array_remove(array, 1);

    printf("{");
    for (size_t i = 0; i < array_size(array); ++i)
    {
        printf("%d, ", array[i]);
    }
    printf("}\n");

    array_swap_remove(array, 1);

    printf("{");
    for (size_t i = 0; i < array_size(array); ++i)
    {
        printf("%d, ", array[i]);
    }
    printf("}\n");

    array_insert(array, 8, 1);

    printf("{");
    for (size_t i = 0; i < array_size(array); ++i)
    {
        printf("%d, ", array[i]);
    }
    printf("}\n");

    array_free(array);

    return 0;
}
