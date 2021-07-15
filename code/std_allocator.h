#pragma once

#include "foundation/core.h"

#include <stdlib.h>

static CF_ALLOCATOR_FUNC(std__realloc)
{
    CF_UNUSED(state);
    CF_UNUSED(old_size);
    CF_UNUSED(align);

    if (new_size) return realloc(memory, new_size);

    free(memory);
    return NULL;
}

static inline CfAllocator
stdAllocator()
{
    return (CfAllocator){.func = std__realloc};
}
