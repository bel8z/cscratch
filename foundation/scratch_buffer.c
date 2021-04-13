#include "scratch_buffer.h"

ScratchBuffer
scratch_create(u8 *memory, usize size)
{
    return (ScratchBuffer){.buf = memory, .cap = size};
}

void *
scratch_alloc(ScratchBuffer *sb, usize size)
{
    // TODO (Matteo): Implement something

    CF_UNUSED(sb);
    CF_UNUSED(size);

    return NULL;
}
