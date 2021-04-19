#include <stdio.h>

#include <windows.h>

#include "foundation/util.h"

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

static inline void
cfRotateReverseBuf(void *array, usize size, usize pos, u8 *swap_buf, usize swap_size)
{
    u8 *buf = array;
    usize rest = size - pos;
    cfReverseBuf(buf, size, swap_buf, swap_size);
    cfReverseBuf(buf, rest, swap_buf, swap_size);
    cfReverseBuf(buf + rest * swap_size, pos, swap_buf, swap_size);
}

#define cfRotateReverse(array, size, pos) \
    cfRotateReverseBuf(array, size, pos, (u8[sizeof(*array)]){0}, sizeof(*array))

enum
{
    COUNT = 268435456,
    POS = COUNT / 4,
};

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
    cfRotateLeft(a, COUNT, POS);
    QueryPerformanceCounter(&end);
    elapsed_us = ((end.QuadPart - start.QuadPart) * 1000000) / freq.QuadPart;
    printf("cfRotateLeft: %zu us\n", elapsed_us);

    QueryPerformanceCounter(&start);
    cfRotateReverse(b, COUNT, POS);
    QueryPerformanceCounter(&end);
    elapsed_us = ((end.QuadPart - start.QuadPart) * 1000000) / freq.QuadPart;
    printf("cfRotateReverse: %zu us\n", elapsed_us);

    for (i32 i = 0; i < COUNT; ++i)
    {
        CF_ASSERT(a[i] == b[i], "!");
        CF_ASSERT(a[i] == (i + POS) % COUNT, "!");
    }

    return 0;
}
