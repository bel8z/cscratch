#include <stdio.h>
#include <stdlib.h>

#include "foundation/array.h"
#include "foundation/core.h"

#include "std_allocator.h"

typedef I32 MyType;
typedef CfArray(MyType) MyArray;

void
arrayPrint(MyArray *a)
{
    FILE *out = stderr;

    fprintf(out, "{");

    for (Usize i = 0; i < a->size; ++i)
    {
        fprintf(out, "%d, ", a->data[i]);
    }

    fprintf(out, "}\n");
    fflush(out);
}

int32_t
main(int32_t argc, char **argv)
{
    (void)argc;
    (void)argv;

    MemAllocator std_alloc = stdAllocator();

    MyArray array = {0};
    cfArrayInit(&array, std_alloc);

    cfArrayPush(&array, 0);
    cfArrayPush(&array, 1);
    cfArrayPush(&array, 2);

    arrayPrint(&array);

    for (Usize i = 0; i < array.size; ++i)
    {
        CF_ASSERT(array.data[i] == (MyType)i, "Array push FAILED");
    }

    CF_ASSERT(cfArrayPop(&array) == 2, "Array pop FAILED");
    CF_ASSERT(cfArrayPop(&array) == 1, "Array pop FAILED");
    CF_ASSERT(cfArrayPop(&array) == 0, "Array pop FAILED");

    arrayPrint(&array);

    CF_ASSERT(cfArrayEmpty(&array), "Array should be empty");

    cfArrayPush(&array, 0);
    cfArrayPush(&array, 1);
    cfArrayPush(&array, 2);
    cfArrayPush(&array, 3);
    cfArrayPush(&array, 4);

    arrayPrint(&array);

    for (Usize i = 0; i < array.size; ++i)
    {
        CF_ASSERT(array.data[i] == (MyType)i, "");
    }

    cfArrayRemove(&array, 1);

    arrayPrint(&array);

    MyType test_remove[] = {0, 2, 3, 4};

    for (Usize i = 0; i < CF_ARRAY_SIZE(test_remove); ++i)
    {
        CF_ASSERT(array.data[i] == test_remove[i], "Array remove FAILED");
    }

    cfArraySwapRemove(&array, 1);

    arrayPrint(&array);

    MyType test_swap_remove[] = {0, 4, 3};

    for (Usize i = 0; i < CF_ARRAY_SIZE(test_swap_remove); ++i)
    {
        CF_ASSERT(array.data[i] == test_swap_remove[i], "Array swap remove FAILED");
    }

    cfArrayInsert(&array, 1, 8);

    arrayPrint(&array);

    MyType test_insert[] = {0, 8, 4, 3};

    for (Usize i = 0; i < CF_ARRAY_SIZE(test_insert); ++i)
    {
        CF_ASSERT(array.data[i] == test_insert[i], "Array insert FAILED");
    }

    cfArrayShutdown(&array);

    return 0;
}
