#pragma once

/// Foundation dynamic array implementation
/// This is not an API header, include it in implementation files only

#include "core.h"
#include "memory.h"

//-------------//
//   CfArray   //
//-------------//

#define cf__arrayNextCap(array, req) cfMax(req, (array)->cap ? 2 * (array)->cap : 1)
#define cf__arrayByteCount(array, count) (count * sizeof(*(array)->buf))

#define cfArrayInit(array, allocator) \
    do                                \
    {                                 \
        (array)->alloc = (allocator); \
        (array)->buf = 0;             \
        (array)->cap = 0;             \
        (array)->len = 0;             \
    } while (0)

#define cfArrayInitCap(array, allocator, capacity)                                \
    do                                                                            \
    {                                                                             \
        (array)->alloc = (allocator);                                             \
        (array)->buf = cfAlloc((allocator), cf__arrayByteCount(array, capacity)); \
        (array)->cap = (capacity);                                                \
        (array)->len = 0;                                                         \
    } while (0)

#define cfArrayFree(array) \
    cfFree((array)->alloc, (array)->buf, cf__arrayByteCount(array, (array)->cap))

/// Size of the stored items in bytes (useful for 'memcpy' and the like)
#define cfArrayBytes(array) cf__arrayByteCount(array, (array)->len)

#define cfArrayEmpty(array) ((array)->len == 0)

#define cfArrayFull(array) ((array)->len == (array)->cap)

/// Pointer to the first element of the array
#define cfArrayFirst(array) ((array)->buf)
/// Pointer to the last element of the array
#define cfArrayLast(array) (cfArrayEnd(array) - 1)
/// Pointer to one past the last element of the array
#define cfArrayEnd(array) ((array)->buf + (array)->len)

#define cfArrayClear(array) ((array)->len = 0)

/// Ensure the array have the requested capacity by growing it if needed
// TODO (Matteo): Geometric growth here too?
#define cfArrayEnsure(array, required_cap)                                                       \
    ((array)->cap < required_cap                                                                 \
         ? ((array)->buf =                                                                       \
                cfRealloc((array)->alloc, (array)->buf, cf__arrayByteCount(array, (array)->cap), \
                          cf__arrayByteCount(array, cf__arrayNextCap(array, required_cap))),     \
            (array)->cap = cf__arrayNextCap(array, required_cap))                                \
         : 0)

/// Reserve capacity for the requested room
#define cfArrayReserve(array, room) cfArrayEnsure(array, (array)->len + (room))

/// Resize the array to the given number of elements
#define cfArrayResize(array, size) (cfArrayEnsure(array, size), (array)->len = (size))

/// Resize the array by adding the given amount of (0-initialized) elements
#define cfArrayExtend(array, amount) cfArrayResize((array), ((array)->len + (amount)))

/// Push the given item at the end of the array
#define cfArrayPush(array, item) (cfArrayReserve(array, 1), (array)->buf[(array)->len++] = item)

/// Pop and return the last element of the array
#define cfArrayPop(array) ((array)->buf[--(array)->len])

/// Insert the given item at the given position in the array
#define cfArrayInsert(array, index, item)                           \
    do                                                              \
    {                                                               \
        cfArrayPush(array, item);                                   \
        cfMemCopy((array)->buf + index, (array)->buf + index + 1,   \
                  cf__arrayByteCount(array, (array)->len - index)); \
        (array)->buf[index] = item;                                 \
    } while (0)

/// Remove the item at the given position in the array
/// The items after the removed item are relocated
#define cfArrayRemove(array, index)                                 \
    do                                                              \
    {                                                               \
        cfMemCopy((array)->buf + index + 1, (array)->buf + index,   \
                  cf__arrayByteCount(array, (array)->len - index)); \
        (array)->len--;                                             \
    } while (0)

/// Remove the item at the given position in the array, without relocation (the
/// last element of the array takes the place of the removed item)
#define cfArraySwapRemove(array, index) ((array)->buf[(index)] = cfArrayPop(array))

// TODO (Matteo):
// * Range insertion/removal
// * Trim unused memory

//--------------//
//   CfBuffer   //
//--------------//

#define cfBufferEmpty(buff) ((buff)->len == 0)
#define cfBufferFull(buff) ((buff)->len == CF_ARRAY_SIZE((buff)->buf))

#define cfBufferPush(buff, item) \
    (CF_ASSERT(!cfBufferFull(buff), "buffer is full"), (buff)->buf[(buff)->len++] = item)

#define cfBufferPop(buff) \
    (CF_ASSERT(!cfBufferEmpty(buff), "buffer is empty"), (buff)->buf[--(buff)->len])

#define cfBufferResize(buff, size) \
    (CF_ASSERT(size < CF_ARRAY_SIZE((buff)->buf), "Not enough buffer capacity"), (buff)->len = size)

#define cfBufferExtend(buff, amount) cfBufferResize((buff), ((buff)->len + (amount)))

// TODO (Matteo):
// * Range insertion/removal
