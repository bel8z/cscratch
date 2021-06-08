#include "scratch_buffer.h"

cfScratchBuffer
cfScratchCreate(U8 *memory, Usize size)
{
    return (cfScratchBuffer){.buf = memory, .cap = size};
}

void *
cfScratchAlloc(cfScratchBuffer *sb, Usize size)
{
    // TODO (Matteo): Implement something
    CF_NOT_IMPLEMENTED();

    CF_UNUSED(sb);
    CF_UNUSED(size);

    return NULL;
}
