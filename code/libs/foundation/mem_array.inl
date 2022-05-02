#pragma once

/// Foundation dynamic array implementation
/// This is not an API header, include it in implementation files only

// TODO (Matteo):
// * Range insertion/removal
// * Trim unused memory

#include "core.h"
#include "error.h"
#include "memory.h"

//=== Non-allocating API ===//

#define memArrayClear(array) ((array)->size = 0)

#define memArrayBytes(array) ((array)->size * sizeof((array)->data))

#define memArrayResize(array, required) \
    ((required) < (array)->capacity ? ((array)->size = (required), Error_None) : Error_BufferFull)

#define memArrayExtend(array, slice_size, out_slice)                          \
    ((array)->size + slice_size < (array)->capacity                           \
         ? (((out_slice) ? *(out_slice) = (array)->data + (array)->size : 0), \
            (array)->size += slice_size, Error_None)                          \
         : Error_BufferFull)

#define memArrayPush(array, item)                          \
    ((array)->size == (array)->capacity ? Error_BufferFull \
                                        : ((array)->data[(array)->size++] = (item), Error_None))

#define memArrayPop(array, item_ptr) \
    ((array)->size ? (*(item_ptr) = (array)->data[--(array)->size], Error_None) : Error_BufferEmpty)

#define memArrayInsert(array, at, item)                                                         \
    (memArrayResize((array), (array)->size + 1)                                                 \
         ? Error_BufferFull                                                                     \
         : (memCopyArray((array)->data + (at), (array)->data + (at) + 1, (array)->size - (at)), \
            (array)->data[at] = (item), Error_None))

#define memArraySwapRemove(array, at)         \
    ((at) >= (array)->size ? Error_OutOfRange \
                           : (memArrayPop((array), &(array)->data[(at)]), Error_None))

#define memArrayStableRemove(array, at)                                                           \
    ((at) >= (array)->size                                                                        \
         ? Error_OutOfRange                                                                       \
         : (memCopyArray((array)->data + (at) + 1, (array)->data + (at), --(array)->size - (at)), \
            Error_None))

//=== Allocating API ===//

#define memArrayFree(array, allocator) \
    ((array)->size = 0, (void)memFreeArray(allocator, (array)->data, (array)->capacity))

#define memArrayEnsure(array, required_capacity, allocator)                              \
    mem__ArrayGrow((void **)(&(array)->data), &(array)->capacity, sizeof((array)->data), \
                   required_capacity, allocator)

#define memArrayReserve(array, room, allocator) \
    memArrayEnsure(array, ((array)->size + room), allocator)

#define memArrayResizeAlloc(array, required, allocator)             \
    (memArrayEnsure(array, required, allocator) ? Error_OutOfMemory \
                                                : memArrayResize(array, required))

#define memArrayExtendAlloc(array, slice_size, allocator, out_slice) \
    (memArrayEnsure(array, (array)->size + (slice_size), allocator)  \
         ? Error_OutOfMemory                                         \
         : memArrayExtend(array, slice_size, out_slice))

#define memArrayPushAlloc(array, item, allocator)                            \
    (memArrayEnsure(array, (array)->size + 1, allocator) ? Error_OutOfMemory \
                                                         : memArrayPush(array, item))

#define memArrayInsertAlloc(array, at, item, allocator)                      \
    (memArrayEnsure(array, (array)->size + 1, allocator) ? Error_OutOfMemory \
                                                         : memArrayInsert(array, at, item))

CF_INTERNAL ErrorCode32
mem__ArrayGrow(void **data, Usize *cap, Usize elem_size, Usize required, MemAllocator allocator)
{
    CF_ASSERT_NOT_NULL(data);
    CF_ASSERT_NOT_NULL(cap);

    if (required > *cap)
    {
        Usize new_cap = memGrowArrayCapacity(*cap);
        CF_ASSERT(cfIsPowerOf2(new_cap), "Capacity not a power of 2");
        while (new_cap < required) new_cap <<= 1;

        void *new_data = memRealloc(allocator, *data, *cap * elem_size, new_cap * elem_size);
        if (!new_data) return Error_OutOfMemory;

        *data = new_data;
        *cap = new_cap;
    }

    return Error_None;
}
