#include "log.h"
#include "error.h"
#include "memory.h"
#include "strings.h"

CfLog
cfLogCreate(VMemApi *vmem, Usize buffer_size)
{
    VMemMirrorBuffer buffer = vmemMirrorAllocate(vmem, buffer_size);
    return (CfLog){
        .os_handle = buffer.os_handle,
        .buffer = buffer.data,
        .size = buffer.size,
    };
}

void
cfLogDestroy(CfLog *log, VMemApi *vmem)
{
    VMemMirrorBuffer buffer = {
        .os_handle = log->os_handle,
        .data = log->buffer,
        .size = log->size,
    };
    vmemMirrorFree(vmem, &buffer);
    memClearStruct(log);
}

void
cfLogAppend(CfLog *log, Str string)
{
    Char8 *ptr = (Char8 *)log->buffer + (log->write_pos & (log->size - 1));
    memCopy(string.ptr, ptr, string.len);
    log->write_pos += string.len;
}

void
cfLogAppendC(CfLog *log, Cstr cstring)
{
    cfLogAppend(log, strFromCstr(cstring));
}

void
cfLogAppendF(CfLog *log, Cstr format, ...)
{
    Char8 *ptr = (Char8 *)log->buffer + (log->write_pos & (log->size - 1));

    va_list args, copy;
    va_start(args, format);
    va_copy(copy, args);

    Usize len = (Usize)vsnprintf(NULL, 0, format, copy); // NOLINT
    va_end(copy);

    CF_ASSERT(len < log->size, "What?");

    vsnprintf(ptr, len + 1, format, args); // NOLINT
    va_end(args);

    log->write_pos += len;
}

Str
cfLogString(CfLog *log)
{
    Str result = {
        .ptr = log->buffer,
        .len = log->write_pos,
    };

    if (log->write_pos > log->size)
    {
        result.ptr += ((log->write_pos + 1) & (log->size - 1));
        result.len = log->size - 1;
    }

    return result;
}

Cstr
cfLogCstring(CfLog *log)
{
    Usize offset = 0;

    if (log->write_pos > log->size)
    {
        offset = ((log->write_pos + 1) & (log->size - 1));
    }

    return (Char8 *)log->buffer + offset;
}

void
cfLogClear(CfLog *log)
{
    log->write_pos = 0;
}
