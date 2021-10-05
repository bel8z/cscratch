#include "foundation/common.h"

enum
{
    MAX_INDICES = 1024
};

typedef struct Sparse
{
    U32 sparse[MAX_INDICES];
    U32 dense[MAX_INDICES];

    U32 next;
    U32 count;
} Sparse;

bool
sparse_has(Sparse const *sparse, U32 index)
{
    U32 dense_index = sparse->sparse[index];
    return dense_index < sparse->count && sparse->dense[dense_index] == index;
}

U32
sparse_add(Sparse *sparse)
{
    sparse->dense[sparse->count] = sparse->next;
    sparse->sparse[sparse->next] = sparse->count++;

    return sparse->next++;
}

void
sparse_remove(Sparse *sparse, U32 index)
{
    if (sparse->count < 1) return;

    U32 dense_index = sparse->sparse[index];

    if (index < sparse->count && sparse->dense[dense_index] == index)
    {
        U32 sparse_index = sparse->dense[--sparse->count];
        sparse->sparse[sparse_index] = dense_index;
        sparse->dense[dense_index] = sparse_index;
    }
}

I32
platformMain(Platform *platform, CommandLine cmd_line)
{
    Sparse sparse = {0};

    U32 a = sparse_add(&sparse);

    CF_ASSERT(sparse_has(&sparse, a), "");

    U32 b = sparse_add(&sparse);

    CF_ASSERT(sparse_has(&sparse, b), "");

    sparse_remove(&sparse, a);

    CF_ASSERT(!sparse_has(&sparse, a), "");

    return 0;
}
