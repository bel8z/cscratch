#pragma once

#include "core.h"

typedef struct cfScratchBuffer
{
    U8 *buf;
    Usize cap;
    Usize beg;
    Usize len;
} cfScratchBuffer;

cfScratchBuffer cfScratchCreate(U8 *memory, Usize size);

void *cfScratchAlloc(cfScratchBuffer *sb, Usize size);
