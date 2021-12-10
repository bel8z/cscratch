#pragma once

#include "core.h"

// Forward declare supported stream types
typedef struct File File;

/// Error codes for IO operations, can be expanded for custom implementation
typedef U32 IoError32;
#define IO_SUCCESS 0
#define IO_STREAM_END 1
#define IO_FILE_ERROR 2

#define IO_FILL(name) IoError32 name(IoReader *self)

/// Offers buffered read over a generic underlying source
typedef struct IoReader IoReader;
struct IoReader
{
    /// (Optional) underlying source
    void *source;
    /// Start of the reader buffer
    U8 *start;
    /// End of the reader buffer
    U8 *end;
    /// Current reading position in the buffer
    U8 *cursor;
    /// Refills the reader buffer from the underlying source
    IO_FILL((*fill));
    /// "Sticky" error code
    IoError32 error_code;
};

/// Initialize a buffered reader over a file
void ioReaderInitFile(IoReader *reader, File *file, U8 *buffer, Usize buffer_size);

/// Initialize a reader over a memory buffer
void ioReaderInitMemory(IoReader *reader, U8 *buffer, Usize buffer_size);

/// Read at most 'count' bytes into the given buffer
/// Buffer can be null (in the case the read bytes are consumed)
Usize ioRead(IoReader *reader, Usize count, U8 *buffer);

Usize ioReadLine(IoReader *reader, Usize count, U8 *buffer);

// TODO (Matteo):
// * read until delimiter
// * read line (special delimiter case)
// * buffered write
// * standard io
