// Misc utilities (basically wrapper on common standard library functions as memset and memcpy)
#ifndef UTIL_H

#include "common.h"

#include <string.h>

static inline void
cfMemClear(u8 *mem, usize count)
{
    memset(mem, 0, count); // NOLINT
}

static inline void
cfMemWrite(u8 *mem, u8 value, usize count)
{
    memset(mem, value, count); // NOLINT
}

static inline void
cfMemCopy(u8 const *from, u8 *to, usize count)
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
    cfMemCopy(l, buf, size);
    cfMemCopy(r, l, size);
    cfMemCopy(buf, r, size);
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
    u8 *lbuf = (u8 *)array + l * item_size;
    u8 *rbuf = (u8 *)array + r * item_size;

    for (usize i = 0; i < count; ++i, lbuf += item_size, rbuf += item_size)
    {
        cfSwapBytes(lbuf, rbuf, item_size);
    }
}

// Defines a temporary buffer object that can hold either a or b, ensuring
// they have the same size.
#define CF_TEMP_SWAP_BUF(a, b) \
    (u8[sizeof(*(1 ? &(a) : &(b)))]) { 0 }

#define cfSwapItem(a, b) cfSwapBuf(&(a), &(b), CF_TEMP_SWAP_BUF(a, b), sizeof(a))

//------------------------------------------------------------------------------
// Array reversal
//------------------------------------------------------------------------------

static inline void
cfReverseBuf(void *array, usize size, u8 *swap_buf, usize swap_size)
{
    u8 *buf = array;
    for (usize i = 0; i < size / 2; ++i)
    {
        cfSwapBuf(buf + i * swap_size, buf + (size - i - 1) * swap_size, swap_buf, swap_size);
    }
}

#define cfReverse(array, size) cfReverseBuf(array, size, (u8[sizeof(*array)]){0}, sizeof(*array))

//------------------------------------------------------------------------------
// Array rotation
//------------------------------------------------------------------------------

// Rotate array using reversal method (from John Bentley's "Programming Pearls")
static inline void
cf__rotateReversal(void *array, usize size, usize pos, u8 *swap_buf, usize swap_size)
{
    u8 *buf = array;
    usize rest = size - pos;
    cfReverseBuf(buf, size, swap_buf, swap_size);
    cfReverseBuf(buf, rest, swap_buf, swap_size);
    cfReverseBuf(buf + rest * swap_size, pos, swap_buf, swap_size);
}

#define cfRotateReversal(array, size, pos) \
    cf__rotateReversal(array, size, pos, (u8[sizeof(*array)]){0}, sizeof(*array))

// Rotate array elements
#define cfRotateLeft(array, size, pos) cfRotateReversal(array, size, pos)
#define cfRotateRight(array, size, pos) cfRotateReversal(array, size, size - pos)

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
