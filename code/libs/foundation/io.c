#include "io.h"

#include "error.h"
#include "memory.h"

//=== IO buffered reader ===//

static inline void
io_fillCondition(IoReader *self)
{
    CF_ASSERT(self->cursor == self->end, "Only an exhausted buffer can be refilled");
}

static IO_FILL_FN(io_fillZero)
{
    io_fillCondition(self);

    static U8 zeros[256] = {0};

    self->start = self->cursor = zeros;
    self->end = zeros + CF_ARRAY_SIZE(zeros);

    return self->error_code;
}

static ErrorCode32
io_readFail(IoReader *self, ErrorCode32 cause)
{
    self->error_code = cause;
    self->fill = io_fillZero;
    return self->error_code;
}

// static IO_FILL_FN(io_fillFromFile)
// {
//     io_fillCondition(self);

//     if (!self->error_code)
//     {
//         File *file = self->source;
//         Size buffer_size = self->end - self->start;
//         Size read_size = file->read(file, self->start, buffer_size);

//         switch (read_size)
//         {
//             case USIZE_MAX:
//                 CF_ASSERT(file->error, "File should report error if read failed");
//                 io_readFail(self, Error_FileError);
//                 break;

//             case 0:
//                 CF_ASSERT(file->eof, "File should report end of file");
//                 io_readFail(self, Error_EndOfStream);
//                 break;

//             default:
//                 // NOTE (Matteo): This is a performance vs. ease of use tradeoff;
//                 // smaller reads are moved at the end of the buffer
//                 Size offset = buffer_size - read_size;
//                 self->cursor = self->start + offset;
//                 if (offset) memCopy(self->start, self->cursor, read_size);
//                 break;
//         }
//     }

//     return self->error_code;
// }

static IO_FILL_FN(io_fillFromMemory)
{
    io_fillCondition(self);

    if (!self->error_code)
    {
        self->fill = io_fillZero;
        self->error_code = Error_EndOfStream;
    }

    return self->error_code;
}

// void
// ioReaderInitFile(IoReader *reader, File *file, U8 *buffer, Size buffer_size)
// {
//     reader->error_code = Error_None;
//     reader->source = file;
//     reader->fill = io_fillFromFile;
//     reader->start = buffer;
//     reader->end = buffer + buffer_size;
//     // NOTE (Matteo): Place cursor at the end to trigger a refill
//     reader->cursor = reader->end;
// }

void
ioReaderInitMemory(IoReader *reader, U8 *buffer, Size buffer_size)
{
    reader->error_code = Error_None;
    reader->source = NULL;
    reader->fill = io_fillFromMemory;
    reader->start = reader->cursor = buffer;
    reader->end = buffer + buffer_size;
}

ErrorCode32
ioRead(IoReader *reader, Size count, U8 *buffer, Size *read_size)
{
    Size read = 0;

    while (true)
    {
        Size remaining = count - read;
        if (!remaining) break;

        CF_ASSERT(reader->end >= reader->cursor, "Overflow");
        Size available = (Size)(reader->end - reader->cursor);

        if (available)
        {
            Size copied = cfMin(available, remaining);
            if (buffer) memCopy(reader->cursor, buffer, copied);
            reader->cursor += copied;
            read += copied;
        }
        else if (reader->fill(reader))
        {
            break;
        }
    }

    if (read_size) *read_size = read;
    return reader->error_code;
}

ErrorCode32
ioReadByte(IoReader *reader, U8 *byte)
{
    Size read;
    if (!ioRead(reader, 1, byte, &read))
    {
        CF_ASSERT(1 == read, "Read too much");
    }
    return reader->error_code;
}

ErrorCode32
ioReadLine(IoReader *reader, Size count, U8 *buffer, Size *length)
{
    Size read = 0;
    bool found = false;
    U8 byte = 0;

    while (true)
    {
        *length = read;

        if (found) return Error_None;
        if (read == count) return Error_StreamTooLong;

        if (ioReadByte(reader, &byte)) break;

        switch (byte)
        {
            case '\r': break; // Ignored
            case '\n': found = true; break;
            default: buffer[read++] = byte; break;
        }
    }

    return reader->error_code;
}

//=== File IO ===//

IoFileContent
ioFileReadAll(IoFileApi *api, Str filename, MemAllocator alloc)
{
    IoFileContent content = {0};
    IoFile *file = api->open(filename, IoOpenMode_Read);

    if (file != api->invalid)
    {
        Size file_size = api->size(file);
        Size read_size = file_size;

        content.data = memAlloc(alloc, read_size);

        if (content.data && api->read(file, content.data, read_size) == read_size)
        {
            content.size = read_size;
        }
        else
        {
            memFree(alloc, content.data, read_size);
            content.data = NULL;
        }

        api->close(file);
    }

    return content;
}

#if 0
static Size
findLineBreak(U8 const *buffer, Size size)
{
    bool cr_found = false;

    for (Size pos = 0; pos < size; ++pos)
    {
        switch (buffer[pos])
        {
            case '\r': cr_found = true; break;
            case '\n': return cr_found ? pos - 1 : pos;
            default: cr_found = false; break;
        }
    }

    return USIZE_MAX;
}
#endif
