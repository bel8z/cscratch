#ifndef CF_SCRATCH_BUFFER_H

#include "common.h"

typedef struct cfScratchBuffer
{
    u8 *buf;
    usize cap;
    usize beg;
    usize len;
} cfScratchBuffer;

cfScratchBuffer cfScratchCreate(u8 *memory, usize size);

void *cfScratchAlloc(cfScratchBuffer *sb, usize size);

#define CF_SCRATCH_BUFFER_H
#endif
