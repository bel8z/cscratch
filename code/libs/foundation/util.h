#pragma once

/// Foundation miscellaneous utility
/// This is not an API header, include it in implementation files only

#include "core.h"
#include "error.h"
#include "memory.h"

CF_INTERNAL inline U32
cfRoundUp(U32 block_size, U32 page_size)
{
    CF_ASSERT((page_size & (page_size - 1)) == 0, "Page size is not a power of 2");
    return page_size * ((block_size + page_size - 1) / page_size);
}

//------------------------------------------------------------------------------
// Swap utilities
//------------------------------------------------------------------------------

// Swap elements using a temporary buffer of the same size
// Implementation detail for cfSwapItem
CF_INTERNAL inline void
cfSwapBuf(void *l, void *r, U8 *buf, Usize size)
{
    memCopy(l, buf, size);
    memCopy(r, l, size);
    memCopy(buf, r, size);
}

// Swap elements byte per byte
CF_INTERNAL inline void
cfSwapBytes(void *l, void *r, Usize size)
{
    U8 *lbuf = (U8 *)l;
    U8 *rbuf = (U8 *)r;
    U8 temp;

    while (size--)
    {
        temp = lbuf[size];
        lbuf[size] = rbuf[size];
        rbuf[size] = temp;
    }
}

CF_INTERNAL inline void
cfSwapBlock(void *array, Usize l, Usize r, Usize count, Usize item_size)
{
    U8 *lbuf = (U8 *)array + l * item_size;
    U8 *rbuf = (U8 *)array + r * item_size;

    for (Usize i = 0; i < count; ++i, lbuf += item_size, rbuf += item_size)
    {
        cfSwapBytes(lbuf, rbuf, item_size);
    }
}

// Defines a temporary buffer object that can hold either a or b, ensuring
// they have the same size.
#define CF_TEMP_SWAP_BUF(a, b) \
    (U8[sizeof(*(1 ? &(a) : &(b)))]) { 0 }

#define cfSwapItem(a, b) cfSwapBuf(&(a), &(b), CF_TEMP_SWAP_BUF(a, b), sizeof(a))

//------------------------------------------------------------------------------
// Array reversal
//------------------------------------------------------------------------------

CF_INTERNAL inline void
cfReverseBuf(void *array, Usize size, U8 *swap_buf, Usize swap_size)
{
    U8 *buf = (U8 *)array;
    for (Usize i = 0; i < size / 2; ++i)
    {
        cfSwapBuf(buf + i * swap_size, buf + (size - i - 1) * swap_size, swap_buf, swap_size);
    }
}

#define cfReverse(array, size) \
    cfReverseBuf(array, size, (u8[sizeof(*(array))]){0}, sizeof(*(array)))

//------------------------------------------------------------------------------
// Array rotation
//------------------------------------------------------------------------------

// Rotate array using reversal method (from John Bentley's "Programming Pearls")
CF_INTERNAL inline void
cf__rotateReversal(void *array, Usize size, Usize pos, U8 *swap_buf, Usize swap_size)
{
    U8 *buf = (U8 *)array;
    Usize rest = size - pos;
    cfReverseBuf(buf, size, swap_buf, swap_size);
    cfReverseBuf(buf, rest, swap_buf, swap_size);
    cfReverseBuf(buf + rest * swap_size, pos, swap_buf, swap_size);
}

#define cfRotateReversal(array, size, pos) \
    cf__rotateReversal(array, size, pos, (U8[sizeof(*(array)]){0}, sizeof(*(array))

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
