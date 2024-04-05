
#include "platform.h"

#include "foundation/core.h"
#include "foundation/error.h"
#include "foundation/mem_buffer.inl"
#include "foundation/memory.h"

#include <stdio.h>

typedef MemBuffer(Size) UsizeBuffer;

void
bufferPrint(UsizeBuffer *a)
{
    FILE *out = stderr;

    fprintf(out, "{");

    for (Size i = 0; i < a->len; ++i)
    {
        fprintf(out, "%zu, ", a->ptr[i]);
    }

    fprintf(out, "}\n");
    fflush(out);
}

I32
consoleMain(Platform *platform, CommandLine *cmd_line)
{
    CF_UNUSED(cmd_line);

    MemAllocator std_alloc = platform->heap;

    UsizeBuffer buffer = {0};

    CF_ASSERT(!memBufferPushAlloc(&buffer, 0, std_alloc), "Buffer allocating push FAILED");
    CF_ASSERT(!memBufferPushAlloc(&buffer, 1, std_alloc), "Buffer allocating push FAILED");
    CF_ASSERT(!memBufferPushAlloc(&buffer, 2, std_alloc), "Buffer allocating push FAILED");

    bufferPrint(&buffer);

    for (Size i = 0; i < buffer.len; ++i)
    {
        CF_ASSERT(buffer.ptr[i] == i, "Buffer push FAILED");
    }

    Size popped = 0;
    CF_ASSERT(!memBufferPop(&buffer, &popped) && popped == 2, "Buffer pop FAILED");
    CF_ASSERT(!memBufferPop(&buffer, &popped) && popped == 1, "Buffer pop FAILED");
    CF_ASSERT(!memBufferPop(&buffer, &popped) && popped == 0, "Buffer pop FAILED");

    bufferPrint(&buffer);

    CF_ASSERT(!buffer.len, "Buffer should be empty");

    CF_ASSERT(!memBufferPush(&buffer, 0), "Buffer push FAILED");
    CF_ASSERT(!memBufferPush(&buffer, 1), "Buffer push FAILED");
    CF_ASSERT(!memBufferPush(&buffer, 2), "Buffer push FAILED");
    CF_ASSERT(!memBufferPush(&buffer, 3), "Buffer push FAILED");

    CF_ASSERT(memBufferPush(&buffer, 4), "Buffer push UNEXPECTED SUCCESS");

    CF_ASSERT(!memBufferPushAlloc(&buffer, 4, std_alloc), "Buffer allocating push FAILED");

    bufferPrint(&buffer);

    for (Size i = 0; i < buffer.len; ++i)
    {
        CF_ASSERT(buffer.ptr[i] == i, "");
    }

    memBufferStableRemove(&buffer, 1);

    bufferPrint(&buffer);

    Size test_remove[] = {0, 2, 3, 4};

    for (Size i = 0; i < CF_ARRAY_SIZE(test_remove); ++i)
    {
        CF_ASSERT(buffer.ptr[i] == test_remove[i], "Buffer remove FAILED");
    }

    memBufferSwapRemove(&buffer, 1);

    bufferPrint(&buffer);

    Size test_swap_remove[] = {0, 4, 3};

    for (Size i = 0; i < CF_ARRAY_SIZE(test_swap_remove); ++i)
    {
        CF_ASSERT(buffer.ptr[i] == test_swap_remove[i], "Buffer swap remove FAILED");
    }

    memBufferInsert(&buffer, 1, 8);

    bufferPrint(&buffer);

    Size test_insert[] = {0, 8, 4, 3};

    for (Size i = 0; i < CF_ARRAY_SIZE(test_insert); ++i)
    {
        CF_ASSERT(buffer.ptr[i] == test_insert[i], "Buffer insert FAILED");
    }

    return 0;
}
