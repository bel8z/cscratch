#pragma once

#include "foundation/common.h"

#include <stdlib.h>

static CF_ALLOCATOR_FUNC(std__realloc)
{
    (void)(state);
    (void)(old_size);

    if (new_size) return realloc(memory, new_size);

    free(memory);
    return NULL;
}

static inline cfAllocator
stdAllocator()
{
    return (cfAllocator){.func = std__realloc};
}
