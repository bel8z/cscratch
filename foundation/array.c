#include "array.h"

struct ArrayHeader
{
    Allocator *alloc;
    usize len; // Number of stored items
    usize cap; // Number of storable items
};

inline ArrayHeader *
array_header(void *array)
{
    ASSERT_NOT_NULL(array);
    return (ArrayHeader *)array - 1;
}

static ArrayHeader *
array_header_grow(ArrayHeader *header, usize room, usize item_size)
{
    usize const req_cap = header->len + room;

    if (req_cap > header->cap)
    {
        usize const dbl_cap = header->cap ? header->cap * 2 : 1;
        usize const new_cap = req_cap > dbl_cap ? req_cap : dbl_cap;

        usize const old_size = sizeof(*header) + header->cap * item_size;
        usize const new_size = sizeof(*header) + new_cap * item_size;

        header = REALLOCATE(header->alloc, header, old_size, new_size);
        header->cap = new_cap;
    }

    return header;
}

usize
array_capacity(void *array)
{
    return array_header(array)->cap;
}

usize
array_size(void *array)
{
    return array_header(array)->len;
}

void *
internal__array_init(void *array, ArrayParams const *params)
{
    (void)array; // Unused for now

    Allocator *alloc = params->alloc;
    usize bytes = params->capacity * params->item_size;

    ArrayHeader *header = ALLOCATE(alloc, sizeof(*header) + bytes);

    header->alloc = alloc;
    header->cap = params->capacity;
    header->len = 0;

    return (header + 1);
}

void
internal__array_free(void *array, usize item_size)
{
    ArrayHeader *header = array_header(array);
    DEALLOCATE(header->alloc, header, sizeof(*header) + header->cap * item_size);
}

void *
internal__array_grow(void *array, usize room, usize item_size)
{
    ArrayHeader *header = array_header_grow(array_header(array), room, item_size);
    return (header + 1);
}

void *
internal__array_ensure(void *array, usize capacity, usize item_size)
{
    ArrayHeader *header = array_header(array);

    if (capacity > header->cap)
    {
        header = array_header_grow(header, capacity - header->cap, item_size);
    }

    return (header + 1);
}

void *
internal__array_extend(void *array, usize room, usize item_size)
{
    ArrayHeader *header = array_header_grow(array_header(array), room, item_size);
    header->len++;
    return (header + 1);
}

void *
internal__array_shrink(void *array, usize room)
{
    ASSERT_NOT_EMPTY(array);
    assert(array_header(array)->len >= room);
    array_header(array)->len -= room;
    return array;
}

void *
internal__array_remove(void *array, usize pos, usize item_count, usize item_size)
{
    ArrayHeader *header = array_header(array);

    assert(pos < header->len);
    assert(pos + item_count < header->len);

    usize const items = header->len - pos - item_count;
    usize const bytes = items * item_size;
    u8 *dst = (u8 *)(header + 1) + pos * item_size;

    memory_copy(dst + item_size * item_count, dst, bytes);

    --header->len;

    return (header + 1);
}

void *
internal__array_insert(void *array, usize pos, usize item_count, usize item_size)
{
    ArrayHeader *header = array_header_grow(array_header(array), item_count, item_size);

    assert(pos < header->len);

    usize const items = header->len - pos;
    usize const bytes = items * item_size;

    u8 *src = (u8 *)(header + 1) + pos * item_size;

    memory_copy(src, src + item_size * item_count, bytes);

    ++header->len;

    return (header + 1);
}
