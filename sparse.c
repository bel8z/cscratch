#include "common.h"

#include <stdlib.h>

// enum
// {
//     INDEX_BITS = 24,
//     GENERATION_BITS = 8,
// };

// typedef u32 Entity;

// typedef union EntityIndex
// {
//     u32 index : INDEX_BITS;
//     u32 generation : GENERATION_BITS;
// } EntityIndex;

// STATIC_ASSERT((INDEX_BITS + GENERATION_BITS) / 8 == sizeof(EntityIndex), "Wrong entity size");

typedef struct Registry
{
    u32 *index;
    u32 size;
} Registry;

typedef struct Store
{
    u32 *index;
    u8 *data;
    u32 cap;
    u32 len;
    u32 data_size;
} Store;

void
registry_init(Registry *reg, u32 size)
{
    reg->index = calloc(size, sizeof(*reg->index));
    reg->size = size;
}

void
store_init(Store *store, Registry const *reg, u32 data_size)
{
    store->len = 0;
    store->cap = reg->size;
    store->index = malloc(store->cap * sizeof(*store->index));
    store->data = malloc(store->cap * data_size);
    store->data_size = data_size;

    for (u32 i = 0; i < store->cap; ++i)
    {
        store->index[i] = store->cap;
    }
}

bool
store_add(Store *store, u32 index, void const *data)
{
    if (index >= store->cap) return false;

    if (store->index[index] == store->cap)
    {
    }
}
