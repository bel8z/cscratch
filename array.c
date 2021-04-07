#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "array.h"

struct ArrayHeader
{
    Allocator *alloc;
    size_t len;    // Number of stored items
    size_t cap;    // Number of storable items
    uint8_t buf[]; // Storage in bytes
};

inline ArrayHeader *
array_header(void *array)
{
    ASSERT_NOT_NULL(array);
    return (ArrayHeader *)((uint8_t *)array - offsetof(ArrayHeader, buf));
}

static ArrayHeader *
array_header_grow(ArrayHeader *header, size_t room, size_t item_size)
{
    size_t const req_cap = header->len + room;

    if (req_cap > header->cap)
    {
        size_t const dbl_cap = header->cap ? header->cap * 2 : 1;
        size_t const new_cap = req_cap > dbl_cap ? req_cap : dbl_cap;

        header = header->alloc->reallocate(
            header, sizeof(*header) + new_cap * item_size,
            header->alloc->state);
        header->cap = new_cap;
    }

    return header;
}

size_t
array_capacity(void *array)
{
    return array_header(array)->cap;
}

size_t
array_size(void *array)
{
    return array_header(array)->len;
}

void *
internal__array_init(void *array, ArrayParams const *params)
{
    (void)array; // Unused for now

    Allocator *alloc = params->alloc;
    size_t bytes = params->capacity * params->item_size;

    ArrayHeader *header =
        alloc->allocate(sizeof(*header) + bytes, alloc->state);

    header->alloc = alloc;
    header->cap = params->capacity;
    header->len = 0;

    return header->buf;
}

void
internal__array_free(void *array)
{
    ArrayHeader *header = array_header(array);
    header->alloc->deallocate(header, header->alloc->state);
}

void *
internal__array_grow(void *array, size_t room, size_t item_size)
{
    ArrayHeader *header =
        array_header_grow(array_header(array), room, item_size);
    return header->buf;
}

void *
internal__array_ensure(void *array, size_t capacity, size_t item_size)
{
    ArrayHeader *header = array_header(array);

    if (capacity > header->cap)
    {
        header = array_header_grow(header, capacity - header->cap, item_size);
    }

    return header->buf;
}

void *
internal__array_extend(void *array, size_t room, size_t item_size)
{
    ArrayHeader *header =
        array_header_grow(array_header(array), room, item_size);
    header->len++;
    return header->buf;
}

void *
internal__array_shrink(void *array, size_t room)
{
    ASSERT_NOT_EMPTY(array);
    assert(array_header(array)->len >= room);
    array_header(array)->len -= room;
    return array;
}

void *
internal__array_remove(void *array, size_t pos, size_t item_count,
                       size_t item_size)
{
    ArrayHeader *header = array_header(array);

    assert(pos < header->len);
    assert(pos + item_count < header->len);

    size_t const items = header->len - pos - item_count;
    size_t const bytes = items * item_size;
    uint8_t *dst = header->buf + pos * item_size;

    memmove_s(dst, bytes, dst + item_size * item_count, bytes);

    --header->len;

    return header->buf;
}

void *
internal__array_insert(void *array, size_t pos, size_t item_count,
                       size_t item_size)
{
    ArrayHeader *header =
        array_header_grow(array_header(array), item_count, item_size);

    assert(pos < header->len);

    size_t const items = header->len - pos;
    size_t const bytes = items * item_size;

    uint8_t *src = header->buf + pos * item_size;

    memmove_s(src + item_size * item_count, bytes, src, bytes);

    ++header->len;

    return header->buf;
}
