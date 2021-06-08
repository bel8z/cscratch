#include "array.h"

#include "allocator.h"

typedef struct cfArrayHeader
{
    cfAllocator *alloc;
    Usize len; // Number of stored items
    Usize cap; // Number of storable items
} cfArrayHeader;

inline cfArrayHeader *
arrayHeader(void *array)
{
    CF_ASSERT_NOT_NULL(array);
    return (cfArrayHeader *)array - 1;
}

static cfArrayHeader *
arrayHeaderGrow(cfArrayHeader *header, Usize room, Usize item_size)
{
    Usize const req_cap = header->len + room;

    if (req_cap > header->cap)
    {
        Usize const dbl_cap = header->cap ? header->cap * 2 : 1;
        Usize const new_cap = req_cap > dbl_cap ? req_cap : dbl_cap;

        Usize const old_size = sizeof(*header) + header->cap * item_size;
        Usize const new_size = sizeof(*header) + new_cap * item_size;

        header = cfRealloc(header->alloc, header, old_size, new_size);
        header->cap = new_cap;
    }

    return header;
}

Usize
cfArrayCapacity(void *array)
{
    return arrayHeader(array)->cap;
}

Usize
cfArraySize(void *array)
{
    return arrayHeader(array)->len;
}

void
cfArrayClear(void *array)
{
    arrayHeader(array)->len = 0;
}

void *
cf__arrayInit(void *array, cfArrayParams const *params)
{
    (void)array; // Unused for now

    cfAllocator *alloc = params->alloc;
    Usize bytes = params->capacity * params->item_size;

    cfArrayHeader *header = cfAlloc(alloc, sizeof(*header) + bytes);

    header->alloc = alloc;
    header->cap = params->capacity;
    header->len = 0;

    return (header + 1);
}

void
cf__arrayFree(void *array, Usize item_size)
{
    cfArrayHeader *header = arrayHeader(array);
    cfFree(header->alloc, header, sizeof(*header) + header->cap * item_size);
}

void *
cf__arrayGrow(void *array, Usize room, Usize item_size)
{
    cfArrayHeader *header = arrayHeaderGrow(arrayHeader(array), room, item_size);
    return (header + 1);
}

void *
cf__arrayEnsure(void *array, Usize capacity, Usize item_size)
{
    cfArrayHeader *header = arrayHeader(array);

    if (capacity > header->cap)
    {
        header = arrayHeaderGrow(header, capacity - header->cap, item_size);
    }

    return (header + 1);
}

void *
cf__arrayExtend(void *array, Usize room, Usize item_size)
{
    cfArrayHeader *header = arrayHeaderGrow(arrayHeader(array), room, item_size);
    header->len += room;
    return (header + 1);
}

void *
cf__arrayShrink(void *array, Usize room)
{
    CF_ASSERT(array, "Cannot shrink an empty array");
    assert(arrayHeader(array)->len >= room);
    arrayHeader(array)->len -= room;
    return array;
}

void *
cf__arrayRemove(void *array, Usize pos, Usize item_count, Usize item_size)
{
    cfArrayHeader *header = arrayHeader(array);

    CF_ASSERT(pos < header->len, "Removing out of bounds range");
    CF_ASSERT(pos + item_count < header->len, "Removing out of bounds range");

    Usize const items = header->len - pos - item_count;
    Usize const bytes = items * item_size;
    U8 *dst = (U8 *)(header + 1) + pos * item_size;

    cfMemCopy(dst + item_size * item_count, dst, bytes);

    --header->len;

    return (header + 1);
}

void *
cf__arrayInsert(void *array, Usize pos, Usize item_count, Usize item_size)
{
    cfArrayHeader *header = arrayHeaderGrow(arrayHeader(array), item_count, item_size);

    CF_ASSERT(pos < header->len, "Inserting out of bounds");

    Usize const items = header->len - pos;
    Usize const bytes = items * item_size;

    U8 *src = (U8 *)(header + 1) + pos * item_size;

    cfMemCopy(src, src + item_size * item_count, bytes);

    ++header->len;

    return (header + 1);
}
