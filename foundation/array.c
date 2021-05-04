#include "array.h"

#include "allocator.h"

typedef struct cfArrayHeader
{
    cfAllocator *alloc;
    usize len; // Number of stored items
    usize cap; // Number of storable items
} cfArrayHeader;

inline cfArrayHeader *
arrayHeader(void *array)
{
    CF_ASSERT_NOT_NULL(array);
    return (cfArrayHeader *)array - 1;
}

static cfArrayHeader *
arrayHeaderGrow(cfArrayHeader *header, usize room, usize item_size)
{
    usize const req_cap = header->len + room;

    if (req_cap > header->cap)
    {
        usize const dbl_cap = header->cap ? header->cap * 2 : 1;
        usize const new_cap = req_cap > dbl_cap ? req_cap : dbl_cap;

        usize const old_size = sizeof(*header) + header->cap * item_size;
        usize const new_size = sizeof(*header) + new_cap * item_size;

        header = cfRealloc(header->alloc, header, old_size, new_size);
        header->cap = new_cap;
    }

    return header;
}

usize
cfArrayCapacity(void *array)
{
    return arrayHeader(array)->cap;
}

usize
cfArraySize(void *array)
{
    return arrayHeader(array)->len;
}

void *
cf__arrayInit(void *array, cfArrayParams const *params)
{
    (void)array; // Unused for now

    cfAllocator *alloc = params->alloc;
    usize bytes = params->capacity * params->item_size;

    cfArrayHeader *header = cfAlloc(alloc, sizeof(*header) + bytes);

    header->alloc = alloc;
    header->cap = params->capacity;
    header->len = 0;

    return (header + 1);
}

void
cf__arrayFree(void *array, usize item_size)
{
    cfArrayHeader *header = arrayHeader(array);
    cfFree(header->alloc, header, sizeof(*header) + header->cap * item_size);
}

void *
cf__arrayGrow(void *array, usize room, usize item_size)
{
    cfArrayHeader *header = arrayHeaderGrow(arrayHeader(array), room, item_size);
    return (header + 1);
}

void *
cf__arrayEnsure(void *array, usize capacity, usize item_size)
{
    cfArrayHeader *header = arrayHeader(array);

    if (capacity > header->cap)
    {
        header = arrayHeaderGrow(header, capacity - header->cap, item_size);
    }

    return (header + 1);
}

void *
cf__arrayExtend(void *array, usize room, usize item_size)
{
    cfArrayHeader *header = arrayHeaderGrow(arrayHeader(array), room, item_size);
    header->len++;
    return (header + 1);
}

void *
cf__arrayShrink(void *array, usize room)
{
    CF_ASSERT(array, "Cannot shrink an empty array");
    assert(arrayHeader(array)->len >= room);
    arrayHeader(array)->len -= room;
    return array;
}

void *
cf__arrayRemove(void *array, usize pos, usize item_count, usize item_size)
{
    cfArrayHeader *header = arrayHeader(array);

    CF_ASSERT(pos < header->len, "Removing out of bounds range");
    CF_ASSERT(pos + item_count < header->len, "Removing out of bounds range");

    usize const items = header->len - pos - item_count;
    usize const bytes = items * item_size;
    u8 *dst = (u8 *)(header + 1) + pos * item_size;

    cfMemCopy(dst + item_size * item_count, dst, bytes);

    --header->len;

    return (header + 1);
}

void *
cf__arrayInsert(void *array, usize pos, usize item_count, usize item_size)
{
    cfArrayHeader *header = arrayHeaderGrow(arrayHeader(array), item_count, item_size);

    CF_ASSERT(pos < header->len, "Inserting out of bounds");

    usize const items = header->len - pos;
    usize const bytes = items * item_size;

    u8 *src = (u8 *)(header + 1) + pos * item_size;

    cfMemCopy(src, src + item_size * item_count, bytes);

    ++header->len;

    return (header + 1);
}
