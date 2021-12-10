#include "io.h"

#include "error.h"
#include "fs.h"
#include "memory.h"

static inline void
io_fillCondition(IoReader *self)
{
    CF_ASSERT(self->cursor == self->end, "Only an exhausted buffer can be refilled");
}

static IO_FILL(io_fillZero)
{
    io_fillCondition(self);

    static U8 zeros[256] = {0};

    self->start = self->cursor = zeros;
    self->end = zeros + CF_ARRAY_SIZE(zeros);

    return self->error_code;
}

static IoError32
io_readFail(IoReader *self, IoError32 cause)
{
    self->error_code = cause;
    self->fill = io_fillZero;
    return self->error_code;
}

static IO_FILL(io_fillFromFile)
{
    io_fillCondition(self);

    if (!self->error_code)
    {
        File *file = self->source;
        Usize buffer_size = self->end - self->start;
        Usize read_size = file->read(file, self->start, buffer_size);

        switch (read_size)
        {
            case USIZE_MAX:
                CF_ASSERT(file->error, "File should report error if read failed");
                io_readFail(self, IO_FILE_ERROR);
                break;

            case 0:
                CF_ASSERT(file->eof, "File should report end of file");
                io_readFail(self, IO_STREAM_END);
                break;

            default:
                Usize offset = buffer_size - read_size;
                self->cursor = self->start + offset;
                if (offset) memCopy(self->start, self->cursor, read_size);
                break;
        }
    }

    return self->error_code;
}

static IO_FILL(io_fillFromMemory)
{
    io_fillCondition(self);

    if (!self->error_code)
    {
        self->fill = io_fillZero;
        self->error_code = IO_STREAM_END;
    }

    return self->error_code;
}

void
ioReaderInitFile(IoReader *reader, File *file, U8 *buffer, Usize buffer_size)
{
    reader->error_code = IO_SUCCESS;
    reader->source = file;
    reader->fill = io_fillFromFile;
    reader->start = buffer;
    reader->end = buffer + buffer_size;
    // NOTE (Matteo): Place cursor at the end to trigger a refill
    reader->cursor = reader->end;
}

void
ioReaderInitMemory(IoReader *reader, U8 *buffer, Usize buffer_size)
{
    reader->error_code = IO_SUCCESS;
    reader->source = NULL;
    reader->fill = io_fillFromMemory;
    reader->start = reader->cursor = buffer;
    reader->end = buffer + buffer_size;
}

Usize
ioRead(IoReader *reader, U8 *buffer, Usize buffer_size)
{
    Usize read = 0;

    while (true)
    {
        Usize remaining = buffer_size - read;
        if (!remaining) break;

        Usize avail = reader->end - reader->cursor;

        if (avail)
        {
            Usize count = cfMin(avail, remaining);
            memCopy(reader->cursor, buffer, count);
            reader->cursor += count;
            read += count;
        }
        else if (reader->fill(reader))
        {
            // NOTE (Matteo): The invariant must be kept even if the refill failed
            io_fillCondition(reader);
            break;
        }
    }

    return read;
}
