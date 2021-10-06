#pragma once

#include "core.h"

// Forward declare supported stream types
typedef struct File File;

/// Offers buffered read over a generic underlying stream
typedef struct IoReader
{
    /// Underlying stream
    void *stream;
    /// Function for reading from the underlying stream
    Usize (*readStream)(void *stream, U8 *buffer, Usize buffer_size);
    /// Local buffer
    U8 buffer[4096]; // TODO (Matteo): Allow for a custom sized buffer?
    /// Number of buffered bytes
    Usize len;
    /// Position of the next buffered read
    Usize pos;
} IoReader;

/// Initialize the buffered reader over a file
void ioReaderInitFile(IoReader *reader, File *file);

/// Call when the underlying stream has been repositioned (e.g. file seek) in order
/// to discard the buffered bytes and ensure the next read is synchronized.
void ioReaderResync(IoReader *reader);

/// Read from the underlying stream to the given buffer
Usize ioRead(IoReader *reader, U8 *buffer, Usize buffer_size);

// TODO (Matteo):
// * read until delimiter
// * read line (special delimiter case)
// * buffered write
// * standard io
