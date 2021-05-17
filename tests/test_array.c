#include <stdio.h>
#include <stdlib.h>

#include "foundation/allocator.h"
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
    cfArrayInit(array, alloc);

    cfArrayPush(array, 0);
    cfArrayPush(array, 1);
    cfArrayPush(array, 2);

    for (usize i = 0; i < cfArraySize(array); ++i)
    {
        CF_ASSERT(array[i] == (i32)i, "Array push FAILED");
    }

    CF_ASSERT(cfArrayPop(array) == 2, "Array pop FAILED");
    CF_ASSERT(cfArrayPop(array) == 1, "Array pop FAILED");
    CF_ASSERT(cfArrayPop(array) == 0, "Array pop FAILED");

    CF_ASSERT(cfArrayEmpty(array), "Array should be empty");

    cfArrayPush(array, 0);
    cfArrayPush(array, 1);
    cfArrayPush(array, 2);
    cfArrayPush(array, 3);
    cfArrayPush(array, 4);

    for (usize i = 0; i < cfArraySize(array); ++i)
    {
        CF_ASSERT(array[i] == (i32)i, "");
    }

    cfArrayRemove(array, 1);

    i32 test_remove[] = {0, 2, 3, 4};

    for (usize i = 0; i < CF_ARRAY_SIZE(test_remove); ++i)
    {
        CF_ASSERT(array[i] == test_remove[i], "Array remove FAILED");
    }

    cfArraySwapRemove(array, 1);

    i32 test_swap_remove[] = {0, 4, 3};

    for (usize i = 0; i < CF_ARRAY_SIZE(test_swap_remove); ++i)
    {
        CF_ASSERT(array[i] == test_swap_remove[i], "Array swap remove FAILED");
    }

    cfArrayInsert(array, 8, 1);

    i32 test_insert[] = {0, 8, 4, 3};

    for (usize i = 0; i < CF_ARRAY_SIZE(test_insert); ++i)
    {
        CF_ASSERT(array[i] == test_insert[i], "Array insert FAILED");
    }

    cfArrayFree(array);

    return 0;
}
