#ifndef CF_SCRATCH_BUFFER_H

#include "common.h"

typedef struct cfScratchBuffer
{
    U8 *buf;
    Usize cap;
    Usize beg;
    Usize len;
} cfScratchBuffer;

cfScratchBuffer cfScratchCreate(U8 *memory, Usize size);

void *cfScratchAlloc(cfScratchBuffer *sb, Usize size);

#define CF_SCRATCH_BUFFER_H
#endif
