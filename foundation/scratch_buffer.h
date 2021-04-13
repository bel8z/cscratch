#ifndef CF_SCRATCH_BUFFER_H

#include "common.h"

typedef struct ScratchBuffer
{
    u8 *buf;
    usize cap;
    usize beg;
    usize len;
} ScratchBuffer;

ScratchBuffer scratch_create(u8 *memory, usize size);

void *scratch_alloc(ScratchBuffer *sb, usize size);

#define CF_SCRATCH_BUFFER_H
#endif
