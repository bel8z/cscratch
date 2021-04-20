#include <stdio.h>

#include <windows.h>

#include "foundation/util.h"

// Rotate array using Gries-Mills block swap algorith
// Implementation detail for cfRotateLeft/cfRotateLeft
static inline void
rotateBlockSwap_(void *array, usize size, usize pos, usize item_size)
{
    if (pos == 0 || pos == size) return;

    usize i = pos;
    usize j = size - pos;

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
arrayPrint(i32 const *array, usize count)
{
    printf("{");
    for (usize i = 0; i < count; ++i)
    {
        printf("%d,", array[i]);
    }
    printf("}\n");
}

int
main(void)
{
    LARGE_INTEGER freq, start, end;
    isize elapsed_us;
    QueryPerformanceFrequency(&freq);

    i32 *a = HeapAlloc(GetProcessHeap(), 0, COUNT * sizeof(*a));
    i32 *b = HeapAlloc(GetProcessHeap(), 0, COUNT * sizeof(*b));

    for (i32 i = 0; i < COUNT; ++i)
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

    for (i32 i = 0; i < COUNT; ++i)
    {
        CF_ASSERT(a[i] == b[i], "!");
        CF_ASSERT(a[i] == (i + POS) % COUNT, "!");
    }

    return 0;
}
