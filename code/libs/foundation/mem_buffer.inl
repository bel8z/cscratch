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
#define memBufferClear(buffer) ((buffer)->len = 0)

/// Size of the buffer item type in bytes
#define memBufferItemSize(buffer) sizeof(*(buffer)->ptr)

/// Size of the buffer in bytes
#define memBufferBytes(buffer) ((buffer)->len * memBufferItemSize(buffer))

/// Set the size of the buffer to the required values, unless it is greater than capacity
#define memBufferResize(buffer, required)                 \
    ((Size)(required) > (buffer)->cap ? Error_BufferFull \
                                       : ((buffer)->len = (Size)(required), Error_None))

/// Increase the size of the buffer by the required amount, unless it causes the size to be greater
/// than capacity.
/// Optionally assigns the given pointer to the added slice of the buffer
#define memBufferExtend(buffer, slice_size, out_slice)                        \
    ((buffer)->len + (Size)(slice_size) > (buffer)->cap                      \
         ? Error_BufferFull                                                   \
         : (((out_slice) ? *(out_slice) = (buffer)->ptr + (buffer)->len : 0), \
            (buffer)->len += (Size)(slice_size), Error_None))

/// Push the given element at the end of the buffer, if the capacity allows for it
#define memBufferPush(buffer, item)                    \
    ((buffer)->len == (buffer)->cap ? Error_BufferFull \
                                    : ((buffer)->ptr[(buffer)->len++] = (item), Error_None))

/// Pops the element at the end of the buffer, if present
#define memBufferPop(buffer, item_ptr) \
    ((buffer)->len ? (*(item_ptr) = (buffer)->ptr[--(buffer)->len], Error_None) : Error_BufferEmpty)

// TODO (Matteo): Review this for better resizing
/// Insert the given element at the given position in the buffer, if the capacity allows for it
#define memBufferInsert(buffer, at, item)                                              \
    (memBufferResize((buffer), (buffer)->len + 1)                                      \
         ? Error_BufferFull                                                            \
         : (at >= (buffer)->len                                                        \
                ? Error_OutOfRange                                                     \
                : (memCopyArray((buffer)->ptr + (Size)(at), (buffer)->ptr + (at) + 1, \
                                (buffer)->len - (Size)(at)),                          \
                   (buffer)->ptr[at] = (item), Error_None)))

/// Remove the element at the given position in the buffer, in constant time, filling the hole with
/// the last buffer element. The order of elements is thus not preserved.
#define memBufferSwapRemove(buffer, at)       \
    ((at) >= (buffer)->len ? Error_OutOfRange \
                           : (memBufferPop((buffer), &(buffer)->ptr[(at)]), Error_None))

/// Remove the element at the given position in the buffer, shifting the followin buffer elements to
/// fill the hole. The order of elements is preserved, but cost of the operation is linear.
#define memBufferStableRemove(buffer, at)                                                         \
    ((at) >= (buffer)->len ? Error_OutOfRange                                                     \
                           : (memCopyArray((buffer)->ptr + (at) + 1, (buffer)->ptr + (Size)(at), \
                                           --(buffer)->len - (Size)(at)),                        \
                              Error_None))

//=== Allocating API ===//

/// Free the allocated buffer memory
#define memBufferFree(buffer, allocator)                                                  \
    ((void)memFreeArray(allocator, (buffer)->ptr, (buffer)->cap), /**/ (buffer)->cap = 0, \
     (buffer)->len = 0)

/// Ensure the buffer has the required capacity, allocating memory if needed
#define memBufferEnsure(buffer, required_capacity, allocator)                            \
    mem_BufferGrow((void **)(&(buffer)->ptr), &(buffer)->cap, memBufferItemSize(buffer), \
                   mem_BufferItemAlign(buffer), (Size)(required_capacity), allocator)

// The following operations are equivalent to the non allocating ones, but uses the provided
// allocator to grow the buffer if needed.

#define memBufferReserve(buffer, room, allocator) \
    memBufferEnsure(buffer, ((buffer)->len + room), allocator)

#define memBufferResizeAlloc(buffer, required, allocator)             \
    (memBufferEnsure(buffer, required, allocator) ? Error_OutOfMemory \
                                                  : memBufferResize(buffer, required))

#define memBufferExtendAlloc(buffer, slice_size, allocator, out_slice) \
    (memBufferEnsure(buffer, (buffer)->len + (slice_size), allocator)  \
         ? Error_OutOfMemory                                           \
         : memBufferExtend(buffer, slice_size, out_slice))

#define memBufferPushAlloc(buffer, item, allocator)                            \
    (memBufferEnsure(buffer, (buffer)->len + 1, allocator) ? Error_OutOfMemory \
                                                           : memBufferPush(buffer, item))

#define memBufferInsertAlloc(buffer, at, item, allocator)                      \
    (memBufferEnsure(buffer, (buffer)->len + 1, allocator) ? Error_OutOfMemory \
                                                           : memBufferInsert(buffer, at, item))

#if CF_COMPILER_CLANG
#    define mem_BufferItemAlign(buffer) alignof(*(buffer)->ptr)
#else
#    define mem_BufferItemAlign(buffer) CF_MAX_ALIGN
#endif

static ErrorCode32
mem_BufferGrow(void **data, Size *cap, Size item_size, Size item_align, Size required,
               MemAllocator allocator)
{
    CF_ASSERT_NOT_NULL(data);
    CF_ASSERT_NOT_NULL(cap);

    if (required > *cap)
    {
        Size new_cap = (*cap) ? (*cap << 1) : 1;
        CF_ASSERT(cfIsPowerOf2(new_cap), "Capacity not a power of 2");
        while (new_cap < required) new_cap <<= 1;

        void *new_data =
            memReallocAlign(allocator, *data, *cap * item_size, new_cap * item_size, item_align);
        if (!new_data) return Error_OutOfMemory;

        *data = new_data;
        *cap = new_cap;
    }

    return Error_None;
}
