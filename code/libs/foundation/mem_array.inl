#pragma once

/// Foundation dynamic array implementation
/// This is not an API header, include it in implementation files only

#include "core.h"
#include "memory.h"

#define memArrayGrowCapacity(array, req) cfMax(req, memGrowArrayCapacity((array)->capacity))

#define memArrayInit(array, allocator) \
    do                                 \
    {                                  \
        (array)->alloc = (allocator);  \
        (array)->data = 0;             \
        (array)->capacity = 0;         \
        (array)->size = 0;             \
    } while (0)

#define memArrayInitCap(array, allocator, init_capacity)                               \
    do                                                                                 \
    {                                                                                  \
        (array)->alloc = (allocator);                                                  \
        (array)->data = 0;                                                             \
        (array)->data = memReallocArray((allocator), (array)->data, 0, init_capacity); \
        (array)->capacity = (init_capacity);                                           \
        (array)->size = 0;                                                             \
    } while (0)

#define memArrayShutdown(array) memFreeArray((array)->alloc, (array)->data, (array)->capacity)

/// Size of the stored items in bytes (useful for 'memcpy' and the like)
#define memArrayBytes(array) ((array)->size * sizeof(*(array)->data))

#define memArrayEmpty(array) ((array)->size == 0)

#define memArrayFull(array) ((array)->size == (array)->capacity)

/// Pointer to the first element of the array
#define memArrayFirst(array) ((array)->data)
/// Pointer to the last element of the array
#define memArrayLast(array) (memArrayEnd(array) - 1)
/// Pointer to one past the last element of the array
#define memArrayEnd(array) ((array)->data + (array)->size)

#define memArrayClear(array) ((array)->size = 0)

/// Ensure the array have the requested capacity by growing it if needed
// TODO (Matteo): Geometric growth here too?
#define memArrayEnsure(array, required_cap)                                                   \
    ((array)->capacity < required_cap                                                         \
         ? ((array)->data = memReallocArray((array)->alloc, (array)->data, (array)->capacity, \
                                            memArrayGrowCapacity(array, required_cap)),       \
            (array)->capacity = memArrayGrowCapacity(array, required_cap))                    \
         : 0)

/// Reserve capacity for the requested room
#define memArrayReserve(array, room) memArrayEnsure(array, (array)->size + (room))

/// Resize the array to the given number of elements
#define memArrayResize(array, new_size) \
    (memArrayEnsure(array, new_size), (array)->size = (new_size))

/// Resize the array by adding the given amount of (0-initialized) elements
#define memArrayExtend(array, amount) memArrayResize((array), ((array)->size + (amount)))

/// Push the given item at the end of the array
#define memArrayPush(array, item) (memArrayReserve(array, 1), (array)->data[(array)->size++] = item)

/// Pop and return the last element of the array
#define memArrayPop(array) ((array)->data[--(array)->size])

/// Insert the given item at the given position in the array
#define memArrayInsert(array, index, item)                                                     \
    do                                                                                         \
    {                                                                                          \
        memArrayPush(array, item);                                                             \
        memCopyArray((array)->data + index, (array)->data + index + 1, (array)->size - index); \
        (array)->data[index] = item;                                                           \
    } while (0)

/// Remove the item at the given position in the array
/// The items after the removed item are relocated
#define memArrayRemove(array, index)                                                           \
    do                                                                                         \
    {                                                                                          \
        memCopyArray((array)->data + index + 1, (array)->data + index, (array)->size - index); \
        (array)->size--;                                                                       \
    } while (0)

/// Remove the item at the given position in the array, without relocation (the
/// last element of the array takes the place of the removed item)
#define memArraySwapRemove(array, index) ((array)->data[(index)] = memArrayPop(array))

// TODO (Matteo):
// * Range insertion/removal
// * Trim unused memory
