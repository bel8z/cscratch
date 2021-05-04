#ifndef CF_ARRAY_H
// -----------------------------------------------------------------------------

#include "common.h"
#include "util.h"

typedef struct cfAllocator cfAllocator;

// -----------------------------------------------------------------------------
// Data types
// -----------------------------------------------------------------------------

typedef struct cfArrayParams
{
    // Mandatory parameters
    cfAllocator *alloc;
    usize item_size;
    // Optional parameters
    usize capacity;
} cfArrayParams;

// Use it to explicitly declare an array (e.g. cfArray(i32) ints;)
#define cfArray(Type) Type *

// -----------------------------------------------------------------------------
// Operations
// -----------------------------------------------------------------------------

/// Initialize a dynamic array with the given allocator and optional capacity
#define cf_array_init(array, allocator, ...)                       \
    (array = cf__arrayInit(array, &(cfArrayParams){                \
                                      .alloc = (allocator),        \
                                      .item_size = sizeof(*array), \
                                      __VA_ARGS__,                 \
                                  }))

/// Free the dynamic array
#define cfArrayFree(array) cf__arrayFree(array, sizeof(*array))

/// Capacity of the array (number of elements that can be stored before the
/// array grows)
usize cfArrayCapacity(void *array);
/// Size of the array (number of stored items)
usize cfArraySize(void *array);
/// Size of the stored items in bytes (useful for 'memcpy' and the like)
#define cfArrayBytes(array) cfArraySize(array) * sizeof(*array))

#define cfArrayFull(array) (cfArraySize(array) == cfArrayCapacity(array))
#define cfArrayEmpty(array) !cfArraySize(array)

/// Pointer to the first element of the array
#define cfArrayFirst(array) array
/// Pointer to the last element of the array
#define cfArrayLast(array) (cfArrayEnd(array) - 1)
/// Pointer to one past the last element of the array
#define cfArrayEnd(array) (array + cfArraySize(array))

/// Grow the array capacity to store at least the given room
/// Growth is geometrical
#define cfArrayGrow(array, room) (array = cf__arrayGrow(array, room, sizeof(*array)))

/// Ensure the array has the given capacity by growing if necessary
#define cfArrayEnsure(array, capacity) (cf__arrayEnsure(array, capacity, sizeof(*array)))

/// Push the given item at the end of the array
#define cfArrayPush(array, item) \
    (array = cf__arrayExtend(array, 1, sizeof(*array)), *cfArrayLast(array) = item)

/// Push the given items at the end of the array
#define cfArrayPushRange(array, items, count)                  \
    do                                                         \
    {                                                          \
        array = cf__arrayExtend(array, count, sizeof(*array)); \
        cfMemCopy(items, array, count * sizeof(*items));       \
    } while (0)

/// Pop and return the last element of the array
#define cfArrayPop(array) (array = cf__arrayShrink(array, 1), *cfArrayEnd(array))

/// Insert the given item at the given position in the array
#define cfArrayInsert(array, item, pos) \
    (array = cf__arrayInsert(array, pos, 1, sizeof(*array)), (array)[pos] = item)

/// Insert the given items at the given position in the array
#define cfArrayInsertRange(array, items, count, pos)                \
    do                                                              \
    {                                                               \
        array = cf__arrayInsert(array, pos, count, sizeof(*array)); \
        cfMemCopy(items, array + pos, count *sizeof(*items) ;       \
    } while (0)

/// Remove the item at the given position in the array
/// The items after the removed item are relocated
#define cfArrayRemove(array, pos) (array = cf__arrayRemove(array, pos, 1, sizeof(*array)))

/// Remove the items at the given position in the array
/// The items after the removed items are relocated
#define cfArrayRemoveRange(array, count, pos) \
    (array = cf__arrayRemove(array, pos, count, sizeof(*array)))

/// Remove the item at the given position in the array, without relocation (the
/// last element of the array takes the place of the removed item)
#define cfArraySwapRemove(array, pos) \
    (array = cf__arrayShrink(array, 1), (array)[pos] = *cfArrayEnd(array))

// TODO
// cf_array_shrink(array)
// cf_array_set_capacity(array, capacity)
// cf_array_resize(array, size)
// cf_array_resize_geom(array, size)

// -----------------------------------------------------------------------------
// Internals
// -----------------------------------------------------------------------------

void *cf__arrayInit(void *array, cfArrayParams const *params);
void cf__arrayFree(void *array, usize item_size);

void *cf__arrayGrow(void *array, usize room, usize item_size);
void *cf__arrayEnsure(void *array, usize capacity, usize item_size);
void *cf__arrayExtend(void *array, usize room, usize item_size);
void *cf__arrayShrink(void *array, usize room);

void *cf__arrayInsert(void *array, usize pos, usize item_count, usize item_size);
void *cf__arrayRemove(void *array, usize pos, usize item_count, usize item_size);

// -----------------------------------------------------------------------------
#define CF_ARRAY_H
#endif
