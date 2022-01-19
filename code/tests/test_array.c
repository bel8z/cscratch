
#include "platform.h"

#include "foundation/core.h"
#include "foundation/error.h"
#include "foundation/mem_array.inl"

#include <stdio.h>

typedef I32 MyType;
typedef MemArray(MyType) MyArray;

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

I32
platformMain(Platform *platform, CommandLine *cmd_line)
{
    CF_UNUSED(cmd_line);

    MemAllocator std_alloc = platform->heap;

    MyArray array = {0};
    memArrayInit(&array, std_alloc);

    memArrayPush(&array, 0);
    memArrayPush(&array, 1);
    memArrayPush(&array, 2);

    arrayPrint(&array);

    for (Usize i = 0; i < array.size; ++i)
    {
        CF_ASSERT(array.data[i] == (MyType)i, "Array push FAILED");
    }

    CF_ASSERT(memArrayPop(&array) == 2, "Array pop FAILED");
    CF_ASSERT(memArrayPop(&array) == 1, "Array pop FAILED");
    CF_ASSERT(memArrayPop(&array) == 0, "Array pop FAILED");

    arrayPrint(&array);

    CF_ASSERT(memArrayEmpty(&array), "Array should be empty");

    memArrayPush(&array, 0);
    memArrayPush(&array, 1);
    memArrayPush(&array, 2);
    memArrayPush(&array, 3);
    memArrayPush(&array, 4);

    arrayPrint(&array);

    for (Usize i = 0; i < array.size; ++i)
    {
        CF_ASSERT(array.data[i] == (MyType)i, "");
    }

    memArrayRemove(&array, 1);

    arrayPrint(&array);

    MyType test_remove[] = {0, 2, 3, 4};

    for (Usize i = 0; i < CF_ARRAY_SIZE(test_remove); ++i)
    {
        CF_ASSERT(array.data[i] == test_remove[i], "Array remove FAILED");
    }

    memArraySwapRemove(&array, 1);

    arrayPrint(&array);

    MyType test_swap_remove[] = {0, 4, 3};

    for (Usize i = 0; i < CF_ARRAY_SIZE(test_swap_remove); ++i)
    {
        CF_ASSERT(array.data[i] == test_swap_remove[i], "Array swap remove FAILED");
    }

    memArrayInsert(&array, 1, 8);

    arrayPrint(&array);

    MyType test_insert[] = {0, 8, 4, 3};

    for (Usize i = 0; i < CF_ARRAY_SIZE(test_insert); ++i)
    {
        CF_ASSERT(array.data[i] == test_insert[i], "Array insert FAILED");
    }

    memArrayShutdown(&array);

    return 0;
}
