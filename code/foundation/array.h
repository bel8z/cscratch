#pragma once

/// Foundation dynamic array implementation
/// This is not an API header, include it in implementation files only

#include "core.h"
#include "memory.h"

//-------------------------------//
//   Dynamic growth primitives   //
//-------------------------------//

// NOTE (Matteo): Those are the very basics required to implement a dynamic array
// and are offered in case the full-fledged CfArray is not needed or suitable.

#define cfArrayGrowCapacity(array, req) cfMax(req, (array)->capacity ? 2 * (array)->capacity : 1)

#define cfArrayRealloc(allocator, ptr, old_cap, new_cap) \
    memRealloc(allocator, ptr, (old_cap) * sizeof(*(ptr)), (new_cap) * sizeof(*(ptr)))

#define cfArrayFree(allocator, ptr, cap) memFree(allocator, ptr, (cap) * sizeof(*(ptr)))

//----------------------------//
//   CfArray implementation   //
//----------------------------//

#define cfArrayInit(array, allocator) \
    do                                \
    {                                 \
        (array)->alloc = (allocator); \
        (array)->data = 0;            \
        (array)->capacity = 0;        \
        (array)->size = 0;            \
    } while (0)

#define cfArrayInitCap(array, allocator, init_capacity)                               \
    do                                                                                \
    {                                                                                 \
        (array)->alloc = (allocator);                                                 \
        (array)->data = 0;                                                            \
        (array)->data = cfArrayRealloc((allocator), (array)->data, 0, init_capacity); \
        (array)->capacity = (init_capacity);                                          \
        (array)->size = 0;                                                            \
    } while (0)

#define cfArrayShutdown(array) cfArrayFree((array)->alloc, (array)->data, (array)->capacity)

/// Size of the stored items in bytes (useful for 'memcpy' and the like)
#define cfArrayBytes(array) ((array)->size * sizeof(*(array)->data))

#define cfArrayEmpty(array) ((array)->size == 0)

#define cfArrayFull(array) ((array)->size == (array)->capacity)

/// Pointer to the first element of the array
#define cfArrayFirst(array) ((array)->data)
/// Pointer to the last element of the array
#define cfArrayLast(array) (cfArrayEnd(array) - 1)
/// Pointer to one past the last element of the array
#define cfArrayEnd(array) ((array)->data + (array)->size)

#define cfArrayClear(array) ((array)->size = 0)

/// Ensure the array have the requested capacity by growing it if needed
// TODO (Matteo): Geometric growth here too?
#define cfArrayEnsure(array, required_cap)                                                   \
    ((array)->capacity < required_cap                                                        \
         ? ((array)->data = cfArrayRealloc((array)->alloc, (array)->data, (array)->capacity, \
                                           cfArrayGrowCapacity(array, required_cap)),        \
            (array)->capacity = cfArrayGrowCapacity(array, required_cap))                    \
         : 0)

/// Reserve capacity for the requested room
#define cfArrayReserve(array, room) cfArrayEnsure(array, (array)->size + (room))

/// Resize the array to the given number of elements
#define cfArrayResize(array, new_size) (cfArrayEnsure(array, new_size), (array)->size = (new_size))

/// Resize the array by adding the given amount of (0-initialized) elements
#define cfArrayExtend(array, amount) cfArrayResize((array), ((array)->size + (amount)))

/// Push the given item at the end of the array
#define cfArrayPush(array, item) (cfArrayReserve(array, 1), (array)->data[(array)->size++] = item)

/// Pop and return the last element of the array
#define cfArrayPop(array) ((array)->data[--(array)->size])

/// Insert the given item at the given position in the array
#define cfArrayInsert(array, index, item)                                                      \
    do                                                                                         \
    {                                                                                          \
        cfArrayPush(array, item);                                                              \
        memCopyArray((array)->data + index, (array)->data + index + 1, (array)->size - index); \
        (array)->data[index] = item;                                                           \
    } while (0)

/// Remove the item at the given position in the array
/// The items after the removed item are relocated
#define cfArrayRemove(array, index)                                                            \
    do                                                                                         \
    {                                                                                          \
        memCopyArray((array)->data + index + 1, (array)->data + index, (array)->size - index); \
        (array)->size--;                                                                       \
    } while (0)

/// Remove the item at the given position in the array, without relocation (the
/// last element of the array takes the place of the removed item)
#define cfArraySwapRemove(array, index) ((array)->data[(index)] = cfArrayPop(array))

// TODO (Matteo):
// * Range insertion/removal
// * Trim unused memory

//--------------//
//   CfBuffer   //
//--------------//

// TODO (Matteo): Is this API really useful?

#define cfBufferEmpty(buff) ((buff)->size == 0)
#define cfBufferFull(buff) ((buff)->size == CF_ARRAY_SIZE((buff)->data))

#define cfBufferPush(buff, item) \
    (CF_ASSERT(!cfBufferFull(buff), "buffer is full"), (buff)->data[(buff)->size++] = item)

#define cfBufferPop(buff) \
    (CF_ASSERT(!cfBufferEmpty(buff), "buffer is empty"), (buff)->data[--(buff)->size])

#define cfBufferResize(buff, new_size)                                                \
    (CF_ASSERT(new_size < CF_ARRAY_SIZE((buff)->data), "Not enough buffer capacity"), \
     (buff)->size = new_size)

#define cfBufferExtend(buff, amount) cfBufferResize((buff), ((buff)->size + (amount)))

// TODO (Matteo):
// * Range insertion/removal
