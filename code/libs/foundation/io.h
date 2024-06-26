#pragma once

#include "core.h"

// TODO (Matteo):
// * read until delimiter
// * read line (special delimiter case)
// * buffered write
// * standard io

//=== IO buffered reader ===//

#define IO_FILL_FN(name) ErrorCode32 name(IoReader *self)

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
    IO_FILL_FN((*fill));
    /// "Sticky" error code
    ErrorCode32 error_code;
};

/// Initialize a buffered reader over a file
// void ioReaderInitFile(IoReader *reader, IoFile *file, U8 *buffer, Size buffer_size);

/// Initialize a reader over a memory buffer
void ioReaderInitMemory(IoReader *reader, U8 *buffer, Size buffer_size);

/// Read at most 'count' bytes into the given buffer
/// Buffer can be null (in the case the read bytes are consumed)
ErrorCode32 ioRead(IoReader *reader, Size count, U8 *buffer, Size *read);

ErrorCode32 ioReadByte(IoReader *reader, U8 *byte);

/// Read a line of text, of at most 'count' bytes, into the given buffer.
/// If the line length length exceeds the given 'count', the line is truncated and an error
/// is returned.
ErrorCode32 ioReadLine(IoReader *reader, Size count, U8 *buffer, Size *length);

//=== File IO ===//

/// OS file handle
typedef struct IoFile IoFile;

/// Iterator over directory content
typedef struct IoDirectory IoDirectory;

typedef U8 IoOpenMode;
enum IoOpenMode_
{
    IoOpenMode_Read = 1,
    IoOpenMode_Write = 2,
    IoOpenMode_RW = IoOpenMode_Read | IoOpenMode_Write,
    IoOpenMode_Append = IoOpenMode_Write | 4,
};

typedef U8 IoSeekPos;
enum IoSeekPos_
{
    IoSeekPos_Begin = 0,
    IoSeekPos_Current,
    IoSeekPos_End,
};

typedef U32 IoFileAttributes;
enum IoFileAttributes_
{
    IoFileAttributes_Directory = 1,
    IoFileAttributes_Symlink = 2,
};

typedef struct IoFileProperties
{
    SystemTime last_write;
    IoFileAttributes attributes;
    Size size;
    bool exists;
} IoFileProperties;

#define IO_DIRECTORY_NEXT(name) bool name(IoDirectory *self, Str *filename, IoFileProperties *props)
#define IO_DIRECTORY_CLOSE(name) void name(IoDirectory *self)

struct IoDirectory
{
    U8 opaque[1280];

    /// Advance the iterator, assigning the entry name to the given string view.
    /// Returns false in case the iteration is terminated (in such a case, the string view is not
    /// valid).
    // NOTE that the string view is valid until the next call to this function (or the iterator is
    // destroyed) so copy its content explicitly if you need to store it.
    /// File properties are provided optionally (if the props pointer is not null).
    IO_DIRECTORY_NEXT((*next));
    IO_DIRECTORY_CLOSE((*close));
};

#define IO_FILE_COPY(name) bool name(Str source, Str dest, bool overwrite)
#define IO_FILE_OPEN(name) IoFile *name(Str filename, IoOpenMode mode)
#define IO_FILE_CLOSE(name) void name(IoFile *file)
#define IO_FILE_SIZE(name) Size name(IoFile *file)
#define IO_FILE_SEEK(name) Size name(IoFile *file, IoSeekPos pos, Offset offset)
#define IO_FILE_READ(name) Size name(IoFile *file, U8 *buffer, Size buffer_size)
#define IO_FILE_READ_AT(name) Size name(IoFile *file, Size offset, U8 *buffer, Size buffer_size)
#define IO_FILE_WRITE(name) bool name(IoFile *file, U8 const *data, Size data_size)
#define IO_FILE_WRITE_AT(name) bool name(IoFile *file, Size offset, U8 const *data, Size data_size)
#define IO_FILE_PROPERTIES(name) IoFileProperties name(IoFile *file)
#define IO_FILE_PROPERTIES_P(name) IoFileProperties name(Str path)

#define IO_DIRECTORY_OPEN(name) bool name(IoDirectory *self, Str path)

typedef struct IoFileApi
{
    IoFile *invalid;

    IoFile *std_in;
    IoFile *std_out;
    IoFile *std_err;

    IO_FILE_COPY((*copy));

    IO_FILE_OPEN((*open));
    IO_FILE_CLOSE((*close));

    IO_FILE_SIZE((*size));
    IO_FILE_PROPERTIES((*properties));
    IO_FILE_PROPERTIES_P((*propertiesP));

    IO_FILE_SEEK((*seek));

    IO_FILE_READ((*read));
    IO_FILE_READ_AT((*readAt));

    IO_FILE_WRITE((*write));
    IO_FILE_WRITE_AT((*writeAt));

    IO_DIRECTORY_OPEN((*dirOpen));
} IoFileApi;

// NOTE (Matteo): Experimental API

typedef struct IoFileContent
{
    U8 *data;
    Size size;
} IoFileContent;

IoFileContent ioFileReadAll(IoFileApi *api, Str filename, MemAllocator alloc);

#define ioFileWriteStr(api, file, str) (api)->write(file, (U8 const *)str.buf, str.len)
