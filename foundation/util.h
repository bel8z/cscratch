// Misc utilities (basically wrapper on common standard library functions as memset and memcpy)
#ifndef UTIL_H

#include "common.h"

#include <string.h>

static inline void
cf_clear_memory(u8 *mem, usize count)
{
    memset(mem, 0, count); // NOLINT
}

static inline void
cf_write_memory(u8 *mem, u8 value, usize count)
{
    memset(mem, value, count); // NOLINT
}

static inline void
cf_copy_memory(u8 const *from, u8 *to, usize count)
{
    memmove_s(to, count, from, count);
}

//------------------------------------------------------------------------------
// Swap utilities
//------------------------------------------------------------------------------

// Swap elements using a temporary buffer of the same size
// Implementation detail for cfSwapItem
static inline void
cfSwapBuf(void *l, void *r, u8 *buf, usize size)
{
    cf_copy_memory(l, buf, size);
    cf_copy_memory(r, l, size);
    cf_copy_memory(buf, r, size);
}

// Swap elements byte per byte
static inline void
cfSwapBytes(void *l, void *r, usize size)
{
    u8 *lbuf = l;
    u8 *rbuf = r;
    u8 temp;

    while (size--)
    {
        temp = lbuf[size], lbuf[size] = rbuf[size], rbuf[size] = temp;
    }
}

static inline void
cfSwapBlock(void *array, usize l, usize r, usize count, usize item_size)
{
    u8 *buf = array;

    for (usize i = 0; i < count; ++i)
    {
        cfSwapBytes(buf + item_size * (l + i), buf + item_size * (r + i), item_size);
    }
}

#define cfSwapItem(a, b) cfSwapBuf(&(a), &(b), (u8[sizeof *(1 ? &(a) : &(b))]){0}, sizeof(a))

//------------------------------------------------------------------------------
// Array reversal
//------------------------------------------------------------------------------

static inline void
cfReverse(i32 *array, usize size)
{
    for (usize i = 0; i < size / 2; ++i)
    {
        cfSwapItem(array[i], array[size - i - 1]);
    }
}

//------------------------------------------------------------------------------
// Array rotation
//------------------------------------------------------------------------------

// Rotate array using Gries-Mills block swap algorith
// Implementation detail for cfRotateLeft/cfRotateLeft
static inline void
cfBlockSwapRotate(void *array, usize size, usize pos, usize item_size)
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

// Rotate array elements
#define cfRotateLeft(array, size, pos) cfBlockSwapRotate(array, size, pos, sizeof(*array))
#define cfRotateRight(array, size, pos) cfBlockSwapRotate(array, size, size - pos, sizeof(*array))

// Swap two adjacent chunks of the same array
#define cfSwapChunkAdjacent(arr, beg, mid, end) \
    cfRotateLeft(arr + beg, end - beg + 1, mid - beg

// Swap two non-adjacent chunks of the same array
#define cfSwapChunk(arr, l_beg, l_end, r_beg, r_end)        \
    do                                                      \
    {                                                       \
        cfSwapChunkAdjacent(arr, l_end + 1, r_beg, r_end);  \
        cfSwapChunkAdjacent(arr, l_beg, l_end + 1, r_end)); \
    } while (0)

//------------------------------------------------------------------------------

#define UTIL_H
#endif
