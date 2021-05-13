#include "scratch_buffer.h"

cfScratchBuffer
cfScratchCreate(u8 *memory, usize size)
{
    return (cfScratchBuffer){.buf = memory, .cap = size};
}

void *
cfScratchAlloc(cfScratchBuffer *sb, usize size)
{
    // TODO (Matteo): Implement something
    CF_NOT_IMPLEMENTED();

    CF_UNUSED(sb);
    CF_UNUSED(size);

    return NULL;
}
