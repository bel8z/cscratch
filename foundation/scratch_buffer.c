#include "scratch_buffer.h"

cfScratchBuffer
cf_scratch_create(u8 *memory, usize size)
{
    return (cfScratchBuffer){.buf = memory, .cap = size};
}

void *
cf_scratch_alloc(cfScratchBuffer *sb, usize size)
{
    // TODO (Matteo): Implement something
    CF_NOT_IMPLEMENTED();

    CF_UNUSED(sb);
    CF_UNUSED(size);

    return NULL;
}
