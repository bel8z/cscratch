#include "io.h"

#include "error.h"
#include "fs.h"
#include "memory.h"

static Usize
io_fileRead(void *context, U8 *buffer, Usize buffer_size)
{
    return fileRead(context, buffer, buffer_size);
}

static Usize
io_fillBuffer(IoReader *reader)
{
    return reader->readStream(reader->stream, reader->buffer, CF_ARRAY_SIZE(reader->buffer));
}

void
ioReaderInitFile(IoReader *reader, File *file)
{
    reader->stream = file;
    reader->readStream = io_fileRead;
    ioReaderResync(reader);
}

void
ioReaderResync(IoReader *reader)
{
    reader->pos = reader->len = 0;
}

Usize
ioRead(IoReader *reader, U8 *buffer, Usize buffer_size)
{
    CF_ASSERT(reader->pos <= reader->len, "Logic error");

    Usize total = 0;

    if (reader->pos < reader->len)
    {
        total = cfMin(buffer_size, reader->len - reader->pos);
        memCopy(reader->buffer + reader->pos, buffer, total);
        reader->pos += total;
    }

    Usize avail = buffer_size - total;

    if (avail > 0)
    {
        CF_ASSERT(reader->pos == reader->len, "Logic error");
        CF_ASSERT(total < buffer_size, "Logic error");

        Usize bytes_read = io_fillBuffer(reader);
        if (bytes_read == USIZE_MAX)
        {
            // Raw read failed: rollback buffered read and report error
            reader->pos -= total;
            return USIZE_MAX;
        }

        reader->len = bytes_read;
        reader->pos = cfMin(avail, bytes_read);
        memCopy(reader->buffer, buffer + total, reader->pos);
        total += reader->pos;
    }

    return total;
}
