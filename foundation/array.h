#ifndef ARRAY_H
// -----------------------------------------------------------------------------

#include "allocator.h"
#include "common.h"
#include "util.h"

// -----------------------------------------------------------------------------
// Data types
// -----------------------------------------------------------------------------

typedef struct ArrayParams
{
    // Mandatory parameters
    Allocator *alloc;
    usize item_size;
    // Optional parameters
    usize capacity;
} ArrayParams;

typedef struct ArrayHeader ArrayHeader;

// -----------------------------------------------------------------------------
// Operations
// -----------------------------------------------------------------------------

/// Initialize a dynamic array with the given allocator and optional capacity
#define array_init(array, allocator, ...)                                 \
    (array = internal__array_init(array, &(ArrayParams){                  \
                                             .alloc = (allocator),        \
                                             .item_size = sizeof(*array), \
                                             __VA_ARGS__,                 \
                                         }))

/// Free the dynamic array
#define array_free(array) internal__array_free(array, sizeof(*array))

/// Capacity of the array (number of elements that can be stored before the
/// array grows)
usize array_capacity(void *array);
/// Size of the array (number of stored items)
usize array_size(void *array);
/// Size of the stored items in bytes (useful for 'memcpy' and the like)
#define array_bytes(array) (array_size(array) * sizeof(*array))

#define array_is_full(array) (array_size(array) == array_capacity(array))
#define array_is_empty(array) !array_size(array)

/// Pointer to the first element of the array
#define array_first(array) array
/// Pointer to the last element of the array
#define array_last(array) (array_end(array) - 1)
/// Pointer to one past the last element of the array
#define array_end(array) (array + array_size(array))

/// Grow the array capacity to store at least the given room
/// Growth is geometrical
#define array_grow(array, room) (array = internal__array_grow(array, room, sizeof(*array)))

/// Ensure the array has the given capacity by growing if necessary
#define array_ensure(array, capacity) (internal__array_ensure(array, capacity, sizeof(*array)))

/// Push the given item at the end of the array
#define array_push(array, item) \
    (array = internal__array_extend(array, 1, sizeof(*array)), *array_last(array) = item)

/// Push the given items at the end of the array
#define array_push_range(array, items, count)                        \
    do                                                               \
    {                                                                \
        array = internal_array_extend(array, count, sizeof(*array)); \
        memory_copy(items, array, count * sizeof(*items));           \
    } while (0)

/// Pop and return the last element of the array
#define array_pop(array) (array = internal__array_shrink(array, 1), *array_end(array))

/// Insert the given item at the given position in the array
#define array_insert(array, item, pos) \
    (array = internal__array_insert(array, pos, 1, sizeof(*array)), (array)[pos] = item)

/// Insert the given items at the given position in the array
#define array_insert_range(array, items, count, pos)                       \
    do                                                                     \
    {                                                                      \
        array = internal__array_insert(array, pos, count, sizeof(*array)); \
        memory_copy(items, array + pos, count *sizeof(*items) ;            \
    } while (0)

/// Remove the item at the given position in the array
/// The items after the removed item are relocated
#define array_remove(array, pos) (array = internal__array_remove(array, pos, 1, sizeof(*array)))

/// Remove the items at the given position in the array
/// The items after the removed items are relocated
#define array_remove_range(array, count, pos) \
    (array = internal__array_remove(array, pos, count, sizeof(*array)))

/// Remove the item at the given position in the array, without relocation (the
/// last element of the array takes the place of the removed item)
#define array_swap_remove(array, pos) \
    (array = internal__array_shrink(array, 1), (array)[pos] = *array_end(array))

// TODO
// array_shrink(array)
// array_set_capacity(array, capacity)
// array_resize(array, size)
// array_resize_geom(array, size)

// -----------------------------------------------------------------------------
// Invariants
// -----------------------------------------------------------------------------

#define ASSERT_NOT_EMPTY(array) assert(array_size(array) && "Array is empty")
#define ASSERT_EMPTY(array) assert(!array_size(array) && "Array is not empty")

// -----------------------------------------------------------------------------
// Internals
// -----------------------------------------------------------------------------

void *internal__array_init(void *array, ArrayParams const *params);
void internal__array_free(void *array, usize item_size);

void *internal__array_grow(void *array, usize room, usize item_size);
void *internal__array_ensure(void *array, usize capacity, usize item_size);
void *internal__array_extend(void *array, usize room, usize item_size);
void *internal__array_shrink(void *array, usize room);

void *internal__array_insert(void *array, usize pos, usize item_count, usize item_size);
void *internal__array_remove(void *array, usize pos, usize item_count, usize item_size);

// -----------------------------------------------------------------------------
#define ARRAY_H
#endif
