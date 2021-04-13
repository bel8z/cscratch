#ifndef CF_SCRATCH_BUFFER_H

#include "common.h"

typedef struct cfScratchBuffer
{
    u8 *buf;
    usize cap;
    usize beg;
    usize len;
} cfScratchBuffer;

cfScratchBuffer cf_scratch_create(u8 *memory, usize size);

void *cf_scratch_alloc(cfScratchBuffer *sb, usize size);

#define CF_SCRATCH_BUFFER_H
#endif
