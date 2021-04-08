#include "common.h"

#include <stdlib.h>

enum
{
    MAX_INDICES = 1024
};

typedef struct Sparse
{
    u32 sparse[MAX_INDICES];
    u32 dense[MAX_INDICES];

    u32 next;
    u32 count;
} Sparse;

bool
sparse_has(Sparse const *sparse, u32 index)
{
    u32 dense_index = sparse->sparse[index];
    return dense_index < sparse->count && sparse->dense[dense_index] == index;
}

u32
sparse_add(Sparse *sparse)
{
    sparse->dense[sparse->count] = sparse->next;
    sparse->sparse[sparse->next] = sparse->count++;

    return sparse->next++;
}

void
sparse_remove(Sparse *sparse, u32 index)
{
    if (sparse->count < 1) return;

    u32 dense_index = sparse->sparse[index];

    if (index < sparse->count && sparse->dense[dense_index] == index)
    {
        u32 sparse_index = sparse->dense[--sparse->count];
        sparse->sparse[sparse_index] = dense_index;
        sparse->dense[dense_index] = sparse_index;
    }
}

i32
main(void)
{
    Sparse sparse = {0};

    u32 a = sparse_add(&sparse);

    ASSERT(sparse_has(&sparse, a), "");

    u32 b = sparse_add(&sparse);

    ASSERT(sparse_has(&sparse, b), "");

    sparse_remove(&sparse, a);

    ASSERT(!sparse_has(&sparse, a), "");

    return 0;
}
