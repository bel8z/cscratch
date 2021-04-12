#include "array.h"

struct CfArrayHeader
{
    CfAllocator *alloc;
    usize len; // Number of stored items
    usize cap; // Number of storable items
};

inline CfArrayHeader *
array_header(void *array)
{
    CF_ASSERT_NOT_NULL(array);
    return (CfArrayHeader *)array - 1;
}

static CfArrayHeader *
array_header_grow(CfArrayHeader *header, usize room, usize item_size)
{
    usize const req_cap = header->len + room;

    if (req_cap > header->cap)
    {
        usize const dbl_cap = header->cap ? header->cap * 2 : 1;
        usize const new_cap = req_cap > dbl_cap ? req_cap : dbl_cap;

        usize const old_size = sizeof(*header) + header->cap * item_size;
        usize const new_size = sizeof(*header) + new_cap * item_size;

        header = CF_REALLOCATE(header->alloc, header, old_size, new_size);
        header->cap = new_cap;
    }

    return header;
}

usize
cf_array_capacity(void *array)
{
    return array_header(array)->cap;
}

usize
cf_array_size(void *array)
{
    return array_header(array)->len;
}

void *
cfinternal__array_init(void *array, CfArrayParams const *params)
{
    (void)array; // Unused for now

    CfAllocator *alloc = params->alloc;
    usize bytes = params->capacity * params->item_size;

    CfArrayHeader *header = CF_ALLOCATE(alloc, sizeof(*header) + bytes);

    header->alloc = alloc;
    header->cap = params->capacity;
    header->len = 0;

    return (header + 1);
}

void
cfinternal__array_free(void *array, usize item_size)
{
    CfArrayHeader *header = array_header(array);
    CF_DEALLOCATE(header->alloc, header, sizeof(*header) + header->cap * item_size);
}

void *
cfinternal__array_grow(void *array, usize room, usize item_size)
{
    CfArrayHeader *header = array_header_grow(array_header(array), room, item_size);
    return (header + 1);
}

void *
cfinternal__array_ensure(void *array, usize capacity, usize item_size)
{
    CfArrayHeader *header = array_header(array);

    if (capacity > header->cap)
    {
        header = array_header_grow(header, capacity - header->cap, item_size);
    }

    return (header + 1);
}

void *
cfinternal__array_extend(void *array, usize room, usize item_size)
{
    CfArrayHeader *header = array_header_grow(array_header(array), room, item_size);
    header->len++;
    return (header + 1);
}

void *
cfinternal__array_shrink(void *array, usize room)
{
    CF_ASSERT(array, "Cannot shrink an empty array");
    assert(array_header(array)->len >= room);
    array_header(array)->len -= room;
    return array;
}

void *
cfinternal__array_remove(void *array, usize pos, usize item_count, usize item_size)
{
    CfArrayHeader *header = array_header(array);

    CF_ASSERT(pos < header->len, "Removing out of bounds range");
    CF_ASSERT(pos + item_count < header->len, "Removing out of bounds range");

    usize const items = header->len - pos - item_count;
    usize const bytes = items * item_size;
    u8 *dst = (u8 *)(header + 1) + pos * item_size;

    cf_copy_memory(dst + item_size * item_count, dst, bytes);

    --header->len;

    return (header + 1);
}

void *
cfinternal__array_insert(void *array, usize pos, usize item_count, usize item_size)
{
    CfArrayHeader *header = array_header_grow(array_header(array), item_count, item_size);

    CF_ASSERT(pos < header->len, "Inserting out of bounds");

    usize const items = header->len - pos;
    usize const bytes = items * item_size;

    u8 *src = (u8 *)(header + 1) + pos * item_size;

    cf_copy_memory(src, src + item_size * item_count, bytes);

    ++header->len;

    return (header + 1);
}
