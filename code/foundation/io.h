#pragma once

#include "core.h"

typedef struct File File;

typedef Usize (*IoReadFn)(void *context, U8 *buffer, Usize buffer_size);

typedef struct IoReader
{
    // Underlying stream
    void *context;
    IoReadFn readfn;

    // Local buffer
    U8 buffer[4096]; // TODO (Matteo): Allow for a custom sized buffer?
    Usize len, pos;
} IoReader;

void ioReaderInitFile(IoReader *reader, File *file);

Usize ioRead(IoReader *reader, U8 *buffer, Usize buffer_size);
Usize ioReadUntil(IoReader *reader, U8 delimiter, U8 *buffer, Usize buffer_size);
