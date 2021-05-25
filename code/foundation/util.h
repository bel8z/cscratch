// Misc utilities (basically wrapper on common standard library functions as memset and memcpy)
#ifndef UTIL_H

#include "common.h"

//------------------------------------------------------------------------------
// Memory utilities (clear, write, copy)
//------------------------------------------------------------------------------

#include <string.h>

static inline void
cfMemClear(void *mem, usize count)
{
    memset(mem, 0, count); // NOLINT
}

static inline void
cfMemWrite(u8 *mem, u8 value, usize count)
{
    memset(mem, value, count); // NOLINT
}

static inline void
cfMemCopy(void const *from, void *to, usize count)
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
// Math utilities
//------------------------------------------------------------------------------

#include <math.h>

#define cfAbs(X) _Generic((X), default : abs, i64 : llabs, f64 : fabs, f32 : fabsf)(X)

#define cfClamp(val, min_val, max_val) \
    ((val) < (min_val) ? (min_val) : (val) > (max_val) ? (max_val) : (val))

#define cfMin(l, r) ((l) < (r) ? (l) : (r))
#define cfMax(l, r) ((l) > (r) ? (l) : (r))

#define cfCeil(X) _Generic((X), default : ceil, f32 : ceilf)(X)
#define cfFloor(X) _Generic((X), default : floor, f32 : floorf)(X)
#define cfRound(X) _Generic((X), default : round, f32 : roundf)(X)

//========
//  Trig
//========

#define cfCos(X) _Generic((X), default : cos, f32 : cosf)(X)
#define cfSin(X) _Generic((X), default : sin, f32 : sinf)(X)
#define cfTan(X) _Generic((X), default : tan, f32 : tanf)(X)

#define cfCosH(X) _Generic((X), default : cosh, f32 : coshf)(X)
#define cfSinH(X) _Generic((X), default : sinh, f32 : sinhf)(X)
#define cfTanH(X) _Generic((X), default : tanh, f32 : tanhf)(X)

//=================
//  Powers / roots
//=================

#define cfSqrt(X) _Generic((X), default : sqrt, f32 : sqrtf)(X)
#define cfPow(base, xp) _Generic((base, xp), default : pow, f32 : powf)(base, xp)
#define cfSquare(x) ((x) * (x))
#define cfCube(x) ((x) * (x) * (x))
#define cfExp(x) _Generic((x), default : exp, f32 : expf)(base, xp)

//==========
//  Modulo
//==========

#define cfFmod(X, Y) _Generic((X, Y), default : fmod, f32 : fmodf)(X, Y)

//========
//  Lerp
//========

#define cfLerp(x, y, t) _Generic((x, y, t), default : cf__Lerp64, f32 : cf__Lerp32)(x, y, t)

static inline f32
cf__Lerp32(f32 x, f32 y, f32 t)
{
    return x * (1 - t) + y * t;
}

static inline f64
cf__Lerp64(f64 x, f64 y, f64 t)
{
    return x * (1 - t) + y * t;
}

//========
//  GCD
//========

#define cfGcd(a, b) \
    _Generic((a, b), u8 : cf__Gcd_u8, u16 : cf__Gcd_u16, u32 : cf__Gcd_u32, u64 : cf__Gcd_u64)(a, b)

#define CF__GCD(Type)                                                            \
    static inline Type cf__Gcd_##Type(Type a, Type b)                            \
    {                                                                            \
        /* GCD(0, b) == b, */                                                    \
        /* GCD(a, 0) == a, */                                                    \
        /* GCD(0, 0) == 0  */                                                    \
                                                                                 \
        if (a == 0) return b;                                                    \
        if (b == 0) return a;                                                    \
                                                                                 \
        /* Find k, which is the greatest power of 2 that divides both a and b */ \
        Type k = 0;                                                              \
        for (; !((a | b) & 1); ++k)                                              \
        {                                                                        \
            a = a >> 1;                                                          \
            b = b >> 1;                                                          \
        }                                                                        \
                                                                                 \
        /* Divide a by 2 until it becames odd */                                 \
        while (!(a & 1)) a = a >> 1;                                             \
                                                                                 \
        do                                                                       \
        {                                                                        \
            /* Divide b by 2 until it becames odd */                             \
            while (!(b & 1)) b = b >> 1;                                         \
                                                                                 \
            /* Now a and b are both odd. Swap in order to have a <= b, */        \
            /* then set b = b - a (which is even). */                            \
            if (a > b)                                                           \
            {                                                                    \
                Type t = a;                                                      \
                a = b;                                                           \
                b = t;                                                           \
            }                                                                    \
                                                                                 \
            b = (b - a);                                                         \
        } while (b);                                                             \
                                                                                 \
        /* Restore the common factor of 2 */                                     \
        return (Type)(a << k);                                                   \
    }

CF__GCD(u8)
CF__GCD(u16)
CF__GCD(u32)
CF__GCD(u64)

#undef CF__GCD

//------------------------------------------------------------------------------

#define UTIL_H
#endif
