#include "platform.h"

#include "foundation/util.h"
#include "foundation/win32.inl"

#include <stdio.h>

// Rotate array using Gries-Mills block swap algorith
// Implementation detail for cfRotateLeft/cfRotateLeft
static inline void
rotateBlockSwap_(void *array, Size size, Size pos, Size item_size)
{
    if (pos == 0 || pos == size) return;

    Size i = pos;
    Size j = size - pos;

    while (i != j)
    {
        if (i < j)
        {
            cfSwapBlock(array, pos - i, pos - i + j, i, item_size);
            j -= i;
        }
        else
        {
            cfSwapBlock(array, pos - i, pos, j, item_size);
            i -= j;
        }
    }

    cfSwapBlock(array, pos - i, pos, i, item_size);
}

#define rotateBlockSwap(array, size, pos) rotateBlockSwap_(array, size, pos, sizeof(*array))

enum
{
    COUNT = 268435456,
    POS = COUNT / 4,
};

void
arrayPrint(I32 const *array, Size count)
{
    printf("{");
    for (Size i = 0; i < count; ++i)
    {
        printf("%d,", array[i]);
    }
    printf("}\n");
}

I32
platformMain(Platform *platform, CommandLine *cmd_line)
{
    LARGE_INTEGER freq, start, end;
    Offset elapsed_us;
    QueryPerformanceFrequency(&freq);

    I32 *a = HeapAlloc(GetProcessHeap(), 0, COUNT * sizeof(*a));
    I32 *b = HeapAlloc(GetProcessHeap(), 0, COUNT * sizeof(*b));

    for (I32 i = 0; i < COUNT; ++i)
    {
        a[i] = b[i] = i;
    }

    QueryPerformanceCounter(&start);
    rotateBlockSwap(a, COUNT, POS);
    QueryPerformanceCounter(&end);
    elapsed_us = ((end.QuadPart - start.QuadPart) * 1000000) / freq.QuadPart;
    printf("rotate block-swap: %zu us\n", elapsed_us);

    QueryPerformanceCounter(&start);
    cfRotateReversal(b, COUNT, POS);
    QueryPerformanceCounter(&end);
    elapsed_us = ((end.QuadPart - start.QuadPart) * 1000000) / freq.QuadPart;
    printf("rotate reversal: %zu us\n", elapsed_us);

    for (I32 i = 0; i < COUNT; ++i)
    {
        CF_ASSERT(a[i] == b[i], "!");
        CF_ASSERT(a[i] == (i + POS) % COUNT, "!");
    }

    return 0;
}
