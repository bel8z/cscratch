#pragma once

/// Foundation dynamic buffer implementation
/// This is not an API header, include it in implementation files only

// TODO (Matteo):
// * Range insertion/removal
// * Trim unused memory

#include "core.h"
#include "error.h"
#include "memory.h"

//=== Non-allocating API ===//

/// Clear the size of the buffer to 0
#define memBufferClear(buffer) ((buffer)->size = 0)

/// Size of the buffer in bytes
#define memBufferBytes(buffer) ((buffer)->size * sizeof((buffer)->data))

/// Set the size of the buffer to the required values, unless it is greater than capacity
#define memBufferResize(buffer, required) \
    ((required) > (buffer)->capacity ? Error_BufferFull : ((buffer)->size = (required), Error_None))

/// Increase the size of the buffer by the required amount, unless it causes the size to be greater
/// than capacity.
/// Optionally assigns the given pointer to the added slice of the buffer
#define memBufferExtend(buffer, slice_size, out_slice)                          \
    ((buffer)->size + slice_size > (buffer)->capacity                           \
         ? Error_BufferFull                                                     \
         : (((out_slice) ? *(out_slice) = (buffer)->data + (buffer)->size : 0), \
            (buffer)->size += slice_size, Error_None))

/// Push the given element at the end of the buffer, if the capacity allows for it
#define memBufferPush(buffer, item)       \
    ((buffer)->size == (buffer)->capacity \
         ? Error_BufferFull               \
         : ((buffer)->data[(buffer)->size++] = (item), Error_None))

/// Pops the element at the end of the buffer, if present
#define memBufferPop(buffer, item_ptr)                                             \
    ((buffer)->size ? (*(item_ptr) = (buffer)->data[--(buffer)->size], Error_None) \
                    : Error_BufferEmpty)

// TODO (Matteo): Review this for better resizing
/// Insert the given element at the given position in the buffer, if the capacity allows for it
#define memBufferInsert(buffer, at, item)                                                          \
    (memBufferResize((buffer), (buffer)->size + 1)                                                 \
         ? Error_BufferFull                                                                        \
         : (at >= (buffer)->size ? Error_OutOfRange                                                \
                                 : (memCopyArray((buffer)->data + (at), (buffer)->data + (at) + 1, \
                                                 (buffer)->size - (at)),                           \
                                    (buffer)->data[at] = (item), Error_None)))

/// Remove the element at the given position in the buffer, in constant time, filling the hole with
/// the last buffer element. The order of elements is thus not preserved.
#define memBufferSwapRemove(buffer, at)        \
    ((at) >= (buffer)->size ? Error_OutOfRange \
                            : (memBufferPop((buffer), &(buffer)->data[(at)]), Error_None))

/// Remove the element at the given position in the buffer, shifting the followin buffer elements to
/// fill the hole. The order of elements is preserved, but cost of the operation is linear.
#define memBufferStableRemove(buffer, at)                                                     \
    ((at) >= (buffer)->size ? Error_OutOfRange                                                \
                            : (memCopyArray((buffer)->data + (at) + 1, (buffer)->data + (at), \
                                            --(buffer)->size - (at)),                         \
                               Error_None))

//=== Allocating API ===//

/// Free the allocated buffer memory
#define memBufferFree(buffer, allocator)                                \
    ((void)memFreeArray(allocator, (buffer)->data, (buffer)->capacity), \
     /**/ (buffer)->capacity = 0, (buffer)->size = 0)

/// Ensure the buffer has the required capacity, allocating memory if needed
#define memBufferEnsure(buffer, required_capacity, allocator)                                \
    mem__BufferGrow((void **)(&(buffer)->data), &(buffer)->capacity, sizeof((buffer)->data), \
                    required_capacity, allocator)

// The following operations are equivalent to the non allocating ones, but uses the provided
// allocator to grow the buffer if needed.

#define memBufferReserve(buffer, room, allocator) \
    memBufferEnsure(buffer, ((buffer)->size + room), allocator)

#define memBufferResizeAlloc(buffer, required, allocator)             \
    (memBufferEnsure(buffer, required, allocator) ? Error_OutOfMemory \
                                                  : memBufferResize(buffer, required))

#define memBufferExtendAlloc(buffer, slice_size, allocator, out_slice) \
    (memBufferEnsure(buffer, (buffer)->size + (slice_size), allocator) \
         ? Error_OutOfMemory                                           \
         : memBufferExtend(buffer, slice_size, out_slice))

#define memBufferPushAlloc(buffer, item, allocator)                             \
    (memBufferEnsure(buffer, (buffer)->size + 1, allocator) ? Error_OutOfMemory \
                                                            : memBufferPush(buffer, item))

#define memBufferInsertAlloc(buffer, at, item, allocator)                       \
    (memBufferEnsure(buffer, (buffer)->size + 1, allocator) ? Error_OutOfMemory \
                                                            : memBufferInsert(buffer, at, item))

CF_INTERNAL ErrorCode32
mem__BufferGrow(void **data, Usize *cap, Usize elem_size, Usize required, MemAllocator allocator)
{
    CF_ASSERT_NOT_NULL(data);
    CF_ASSERT_NOT_NULL(cap);

    if (required > *cap)
    {
        Usize new_cap = (*cap) ? (*cap << 1) : 1;
        CF_ASSERT(cfIsPowerOf2(new_cap), "Capacity not a power of 2");
        while (new_cap < required) new_cap <<= 1;

        void *new_data = memRealloc(allocator, *data, *cap * elem_size, new_cap * elem_size);
        if (!new_data) return Error_OutOfMemory;

        *data = new_data;
        *cap = new_cap;
    }

    return Error_None;
}
