#include <stdio.h>
#include <stdlib.h>

#include "foundation/common.h"

#include "foundation/allocator.h"
// #include "foundation/array.h"

#include "std_allocator.h"

#define cfArray_(Type)      \
    struct                  \
    {                       \
        cfAllocator *alloc; \
        Type *buf;          \
        Usize len;          \
        Usize cap;          \
    }

#define cfArrayInit(array, allocator) \
    do                                \
    {                                 \
        (array)->alloc = (allocator); \
        (array)->len = 0;             \
        (array)->cap = 0;             \
    } while (0)

#define cfArrayPush(array, item)                                                           \
    do                                                                                     \
    {                                                                                      \
        if ((array)->len == (array)->cap)                                                  \
        {                                                                                  \
            Usize CF__ARRAY_NEW_SIZE = (array)->cap ? (array)->cap << 1 : 2;               \
            (array)->buf =                                                                 \
                cfRealloc((array)->alloc, (array)->buf, (array)->cap, CF__ARRAY_NEW_SIZE); \
            (array)->cap = CF__ARRAY_NEW_SIZE;                                             \
        }                                                                                  \
        (array)->buf[(array)->len++] = item;                                               \
    } while (0)

#define cfArrayPop(array) ((array)->buf[--(array)->len])

#define cfArraySize(array) (array)->len
#define cfArrayEmpty(array) ((array)->len == 0)

#define cfArraySwapRemove(array, index) ((array)->buf[(index)] = cfArrayPop(array))

#define cfArrayFree(array) cfFree((array)->alloc, (array)->buf, (array)->cap)

int32_t
main(int32_t argc, char **argv)
{
    (void)argc;
    (void)argv;

    cfAllocator std_alloc = stdAllocator();
    cfAllocator *alloc = &std_alloc;

    cfArray_(I32) array = {0};
    cfArrayInit(&array, alloc);

    cfArrayPush(&array, 0);
    cfArrayPush(&array, 1);
    cfArrayPush(&array, 2);

    for (Usize i = 0; i < cfArraySize(&array); ++i)
    {
        CF_ASSERT(array.buf[i] == (I32)i, "Array push FAILED");
    }

    CF_ASSERT(cfArrayPop(&array) == 2, "Array pop FAILED");
    CF_ASSERT(cfArrayPop(&array) == 1, "Array pop FAILED");
    CF_ASSERT(cfArrayPop(&array) == 0, "Array pop FAILED");

    CF_ASSERT(cfArrayEmpty(&array), "Array should be empty");

    cfArrayPush(&array, 0);
    cfArrayPush(&array, 1);
    cfArrayPush(&array, 2);
    cfArrayPush(&array, 3);
    cfArrayPush(&array, 4);

    for (Usize i = 0; i < cfArraySize(&array); ++i)
    {
        CF_ASSERT(array.buf[i] == (I32)i, "");
    }

    cfArrayRemove(array, 1);

    I32 test_remove[] = {0, 2, 3, 4};

    for (Usize i = 0; i < CF_ARRAY_SIZE(test_remove); ++i)
    {
        CF_ASSERT(array[i] == test_remove[i], "Array remove FAILED");
    }

    cfArraySwapRemove(&array, 1);

    I32 test_swap_remove[] = {0, 4, 3};

    for (Usize i = 0; i < CF_ARRAY_SIZE(test_swap_remove); ++i)
    {
        CF_ASSERT(array[i] == test_swap_remove[i], "Array swap remove FAILED");
    }

    cfArrayInsert(array, 8, 1);

    I32 test_insert[] = {0, 8, 4, 3};

    for (Usize i = 0; i < CF_ARRAY_SIZE(test_insert); ++i)
    {
        CF_ASSERT(array[i] == test_insert[i], "Array insert FAILED");
    }

    cfArrayFree(&array);

    return 0;
}
