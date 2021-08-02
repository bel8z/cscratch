#include "foundation/core.h"
#include "foundation/memory.h"
#include "foundation/strings.h"

#include "foundation/win32.h"

#include "stdio.h"

// NOTE (Matteo): This buffer is made by two ajacent virtual memory regions that both map to the
// same physical memory.
// This allows for an apparently continuous layout of memory even when writes wrap around the end of
// the buffer.

typedef struct MirrorBuffer
{
    Usize size;
    U8 *data;
    void *os_handle;
} MirrorBuffer;

MirrorBuffer
mirrorAllocate(Usize size)
{
    SYSTEM_INFO info = {0};
    GetSystemInfo(&info);

    // NOTE (Matteo): Size is rounded to virtual memory granularity because the mapping addresses
    // must be aligned as such.
    Usize granularity = info.dwAllocationGranularity;
    Usize buffer_size = (size + granularity - 1) & ~(granularity - 1);

    MirrorBuffer mb = {0};

    HANDLE mapping =
        CreateFileMappingW(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, (DWORD)(buffer_size >> 32),
                           (DWORD)(buffer_size & 0xffffffff), NULL);

    if (mapping)
    {
        mb.size = buffer_size;
        mb.os_handle = mapping;

        while (!mb.data)
        {
            U8 *address = VirtualAlloc(NULL, buffer_size * 2, MEM_RESERVE, PAGE_READWRITE);
            if (address)
            {
                VirtualFree(address, 0, MEM_RELEASE);

                U8 *view1 =
                    MapViewOfFileEx(mapping, FILE_MAP_ALL_ACCESS, 0, 0, buffer_size, address);
                U8 *view2 = MapViewOfFileEx(mapping, FILE_MAP_ALL_ACCESS, 0, 0, buffer_size,
                                            view1 + buffer_size);

                if (view1 && view2)
                {
                    mb.data = view1;
                }
                else
                {
                    win32PrintLastError();
                }
            }
        }

        cfMemClear(mb.data, mb.size);
    }

    return mb;
}

void
mirrorFree(MirrorBuffer *mb)
{
    if (mb->data)
    {
        UnmapViewOfFile(mb->data + mb->size);
        UnmapViewOfFile(mb->data);
    }

    if (mb->os_handle) CloseHandle(mb->os_handle);

    mb->size = 0;
    mb->data = 0;
    mb->os_handle = 0;
}

typedef struct LogBuffer
{
    MirrorBuffer buff;
    Usize write_pos;
} LogBuffer;

LogBuffer
logBufferCreate(Usize size)
{
    return (LogBuffer){.buff = mirrorAllocate(size)};
}

void
logBufferDestroy(LogBuffer *buffer)
{
    mirrorFree(&buffer->buff);
    buffer->write_pos = 0;
}

void
logBufferAppend(LogBuffer *buffer, Str string)
{

    U8 *ptr = buffer->buff.data + (buffer->write_pos & (buffer->buff.size - 1));
    cfMemCopy(string.buf, ptr, string.len);
    buffer->write_pos += string.len;
}

CF_PRINTF_LIKE(1, 2)
void
logBufferAppendF(LogBuffer *buffer, Cstr format, ...)
{
    char *ptr = (char *)buffer->buff.data + (buffer->write_pos & (buffer->buff.size - 1));

    va_list args, copy;
    va_start(args, format);

    va_copy(copy, args);
    va_start(copy, format);
    Usize len = (Usize)vsnprintf(NULL, 0, format, copy);
    va_end(copy);

    CF_ASSERT(len < buffer->buff.size, "What?");

    vsnprintf(ptr, len + 1, format, args);
    va_end(args);

    buffer->write_pos += len;
}

Cstr
logBufferCstring(LogBuffer *buffer)
{
    Usize offset = 0;

    if (buffer->write_pos > buffer->buff.size)
    {
        offset = ((buffer->write_pos + 1) & (buffer->buff.size - 1));
    }

    return (char *)buffer->buff.data + offset;
}

Usize
logBufferSize(LogBuffer *buffer)
{
    return buffer->buff.size;
}

int
main(void)
{
    LogBuffer log = logBufferCreate(1);

    bool repeat = true;

    while (repeat)
    {
        if (log.write_pos > logBufferSize(&log)) repeat = false;
        logBufferAppendF(&log, "Write pos: %zu\n", log.write_pos);
    }

    fprintf(stdout, "%s", logBufferCstring(&log));

    // Cleanup
    logBufferDestroy(&log);

    return 0;
}
