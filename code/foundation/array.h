#pragma once

#include "common.h"

#define cfArrayInit(array, allocator) \
    do                                \
    {                                 \
        (array)->alloc = (allocator); \
        (array)->buf = 0;             \
        (array)->cap = 0;             \
        (array)->cap = 0;             \
    } while (0)

#define cfArrayInitCap(array, allocator, cap)     \
    do                                            \
    {                                             \
        (array)->alloc = (allocator);             \
        (array)->buf = cfAlloc((allocator), cap); \
        (array)->cap = cap;                       \
        (array)->cap = 0;                         \
    } while (0)

#define cfArrayFree(array) \
    cfFree((array)->alloc, (array)->buf, (array)->cap * sizeof(*(array)->buf))

/// Size of the stored items in bytes (useful for 'memcpy' and the like)
#define cfArrayBytes(array) ((array)->len * sizeof(*(array)->buf))

#define cfArrayEmpty(array) ((array)->len == 0)

#define cfArrayFull(array) ((array)->len == (array)->cap)

/// Pointer to the first element of the array
#define cfArrayFirst(array) (array)->buf
/// Pointer to the last element of the array
#define cfArrayLast(array) (cfArrayEnd(array) - 1)
/// Pointer to one past the last element of the array
#define cfArrayEnd(array) ((array)->buf + (array)->len)

#define cfArrayClear(array) ((array)->len = 0)

/// Push the given item at the end of the array
#define cfArrayPush(array, item)                                                              \
    do                                                                                        \
    {                                                                                         \
        if (cfArrayFull(array))                                                               \
        {                                                                                     \
            Usize CF__NEW_CAP = (array)->cap ? (array)->cap << 1 : 2;                         \
            (array)->buf =                                                                    \
                cfRealloc((array)->alloc, (array)->buf, sizeof(*(array)->buf) * (array)->cap, \
                          sizeof(*(array)->buf) * CF__NEW_CAP);                               \
            (array)->cap = CF__NEW_CAP;                                                       \
        }                                                                                     \
        (array)->buf[(array)->len++] = item;                                                  \
    } while (0)

/// Pop and return the last element of the array
#define cfArrayPop(array) ((array)->buf[--(array)->len])

/// Insert the given item at the given position in the array
#define cfArrayInsert(array, index, item)                          \
    do                                                             \
    {                                                              \
        cfArrayPush(array, item);                                  \
        cfMemCopy((array)->buf + index, (array)->buf + index + 1,  \
                  ((array)->len - index) * sizeof(*(array)->buf)); \
        (array)->buf[index] = item;                                \
    } while (0)

/// Remove the item at the given position in the array
/// The items after the removed item are relocated
#define cfArrayRemove(array, index)                                \
    do                                                             \
    {                                                              \
        cfMemCopy((array)->buf + index + 1, (array)->buf + index,  \
                  ((array)->len - index) * sizeof(*(array)->buf)); \
        (array)->len--;                                            \
    } while (0)

/// Remove the item at the given position in the array, without relocation (the
/// last element of the array takes the place of the removed item)
#define cfArraySwapRemove(array, index) ((array)->buf[(index)] = cfArrayPop(array))

#define cfArrayResize(array, size)                                                            \
    do                                                                                        \
    {                                                                                         \
        if ((array)->cap < size)                                                              \
        {                                                                                     \
            (array)->buf =                                                                    \
                cfRealloc((array)->alloc, (array)->buf, sizeof(*(array)->buf) * (array)->cap, \
                          sizeof(*(array)->buf) * size);                                      \
            (array)->cap = size;                                                              \
        }                                                                                     \
        (array)->len = size;                                                                  \
    } while (0)

// TODO (Matteo):
// * Resize with geometric growth
// * Set/grow/reserve capacity
// * Range insertion/removal
// * Trim unused memory
