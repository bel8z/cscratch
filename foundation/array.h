#ifndef CF_ARRAY_H
// -----------------------------------------------------------------------------

#include "allocator.h"
#include "common.h"
#include "util.h"

// -----------------------------------------------------------------------------
// Data types
// -----------------------------------------------------------------------------

typedef struct CfArrayParams
{
    // Mandatory parameters
    CfAllocator *alloc;
    usize item_size;
    // Optional parameters
    usize capacity;
} CfArrayParams;

typedef struct CfArrayHeader CfArrayHeader;

// -----------------------------------------------------------------------------
// Operations
// -----------------------------------------------------------------------------

/// Initialize a dynamic array with the given allocator and optional capacity
#define cf_array_init(array, allocator, ...)                                \
    (array = cfinternal__array_init(array, &(CfArrayParams){                \
                                               .alloc = (allocator),        \
                                               .item_size = sizeof(*array), \
                                               __VA_ARGS__,                 \
                                           }))

/// Free the dynamic array
#define cf_array_free(array) cfinternal__array_free(array, sizeof(*array))

/// Capacity of the array (number of elements that can be stored before the
/// array grows)
usize cf_array_capacity(void *array);
/// Size of the array (number of stored items)
usize cf_array_size(void *array);
/// Size of the stored items in bytes (useful for 'memcpy' and the like)
#define cf_array_bytes(array) cf_array_size(array) * sizeof(*array))

#define cf_array_is_full(array) (cf_array_size(array) == cf_array_capacity(array))
#define cf_array_is_empty(array) !cf_array_size(array)

/// Pointer to the first element of the array
#define cf_array_first(array) array
/// Pointer to the last element of the array
#define cf_array_last(array) (cf_array_end(array) - 1)
/// Pointer to one past the last element of the array
#define cf_array_end(array) (array + cf_array_size(array))

/// Grow the array capacity to store at least the given room
/// Growth is geometrical
#define cf_array_grow(array, room) (array = cfinternal__array_grow(array, room, sizeof(*array)))

/// Ensure the array has the given capacity by growing if necessary
#define cf_array_ensure(array, capacity) (cfinternal__array_ensure(array, capacity, sizeof(*array)))

/// Push the given item at the end of the array
#define cf_array_push(array, item) \
    (array = cfinternal__array_extend(array, 1, sizeof(*array)), *cf_array_last(array) = item)

/// Push the given items at the end of the array
#define cf_array_push_range(array, items, count)                       \
    do                                                                 \
    {                                                                  \
        array = cfinternal_array_extend(array, count, sizeof(*array)); \
        cf_copy_memory(items, array, count * sizeof(*items));          \
    } while (0)

/// Pop and return the last element of the array
#define cf_array_pop(array) (array = cfinternal__array_shrink(array, 1), *cf_array_end(array))

/// Insert the given item at the given position in the array
#define cf_array_insert(array, item, pos) \
    (array = cfinternal__array_insert(array, pos, 1, sizeof(*array)), (array)[pos] = item)

/// Insert the given items at the given position in the array
#define cf_array_insert_range(array, items, count, pos)                      \
    do                                                                       \
    {                                                                        \
        array = cfinternal__array_insert(array, pos, count, sizeof(*array)); \
        cf_copy_memory(items, array + pos, count *sizeof(*items) ;           \
    } while (0)

/// Remove the item at the given position in the array
/// The items after the removed item are relocated
#define cf_array_remove(array, pos) \
    (array = cfinternal__array_remove(array, pos, 1, sizeof(*array)))

/// Remove the items at the given position in the array
/// The items after the removed items are relocated
#define cf_array_remove_range(array, count, pos) \
    (array = cfinternal__array_remove(array, pos, count, sizeof(*array)))

/// Remove the item at the given position in the array, without relocation (the
/// last element of the array takes the place of the removed item)
#define cf_array_swap_remove(array, pos) \
    (array = cfinternal__array_shrink(array, 1), (array)[pos] = *cf_array_end(array))

// TODO
// cf_array_shrink(array)
// cf_array_set_capacity(array, capacity)
// cf_array_resize(array, size)
// cf_array_resize_geom(array, size)

// -----------------------------------------------------------------------------
// Internals
// -----------------------------------------------------------------------------

void *cfinternal__array_init(void *array, CfArrayParams const *params);
void cfinternal__array_free(void *array, usize item_size);

void *cfinternal__array_grow(void *array, usize room, usize item_size);
void *cfinternal__array_ensure(void *array, usize capacity, usize item_size);
void *cfinternal__array_extend(void *array, usize room, usize item_size);
void *cfinternal__array_shrink(void *array, usize room);

void *cfinternal__array_insert(void *array, usize pos, usize item_count, usize item_size);
void *cfinternal__array_remove(void *array, usize pos, usize item_count, usize item_size);

// -----------------------------------------------------------------------------
#define CF_ARRAY_H
#endif
