
#include "platform.h"

#include "foundation/core.h"
#include "foundation/error.h"
#include "foundation/mem_array.inl"
#include "foundation/memory.h"

#include <stdio.h>

typedef I32 MyType;
typedef MemBuffer(MyType) MyArray;

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
    // memArrayInit(&array, std_alloc);

    CF_ASSERT(!memArrayPushAlloc(&array, 0, std_alloc), "Array allocating push FAILED");
    CF_ASSERT(!memArrayPushAlloc(&array, 1, std_alloc), "Array allocating push FAILED");
    CF_ASSERT(!memArrayPushAlloc(&array, 2, std_alloc), "Array allocating push FAILED");

    arrayPrint(&array);

    for (Usize i = 0; i < array.size; ++i)
    {
        CF_ASSERT(array.data[i] == (MyType)i, "Array push FAILED");
    }

    I32 popped;
    CF_ASSERT(!memArrayPop(&array, &popped) && popped == 2, "Array pop FAILED");
    CF_ASSERT(!memArrayPop(&array, &popped) && popped == 1, "Array pop FAILED");
    CF_ASSERT(!memArrayPop(&array, &popped) && popped == 0, "Array pop FAILED");

    arrayPrint(&array);

    CF_ASSERT(!array.size, "Array should be empty");

    CF_ASSERT(!memArrayPush(&array, 0), "Array push FAILED");
    CF_ASSERT(!memArrayPush(&array, 1), "Array push FAILED");
    CF_ASSERT(!memArrayPush(&array, 2), "Array push FAILED");
    CF_ASSERT(!memArrayPush(&array, 3), "Array push FAILED");

    CF_ASSERT(memArrayPush(&array, 4), "Array push UNEXPECTED SUCCESS");

    CF_ASSERT(!memArrayPushAlloc(&array, 4, std_alloc), "Array allocating push FAILED");

    arrayPrint(&array);

    for (Usize i = 0; i < array.size; ++i)
    {
        CF_ASSERT(array.data[i] == (MyType)i, "");
    }

    memArrayStableRemove(&array, 1);

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

    // memArrayShutdown(&array);

    return 0;
}
