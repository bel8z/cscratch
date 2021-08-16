#include "log.h"
#include "strings.h"

#include <stdarg.h>

CfLog
cfLogCreate(CfVirtualMemory *vm, Usize buffer_size)
{
    return (CfLog){.buffer = vmMirrorAllocate(vm, buffer_size)};
}

void
cfLogDestroy(CfLog *log, CfVirtualMemory *vm)
{
    vmMirrorFree(vm, &log->buffer);
}

void
cfLogAppend(CfLog *log, Str string)
{
    Char8 *ptr = (Char8 *)log->buffer.data + (log->write_pos & (log->buffer.size - 1));
    memCopy(string.buf, ptr, string.len);
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
    Char8 *ptr = (Char8 *)log->buffer.data + (log->write_pos & (log->buffer.size - 1));

    va_list args, copy;
    va_start(args, format);
    va_copy(copy, args);

    Usize len = (Usize)vsnprintf(NULL, 0, format, copy); // NOLINT
    va_end(copy);

    CF_ASSERT(len < log->buffer.size, "What?");

    vsnprintf(ptr, len + 1, format, args); // NOLINT
    va_end(args);

    log->write_pos += len;
}

Str
cfLogString(CfLog *log)
{
    Str result = {
        .buf = log->buffer.data,
        .len = log->write_pos,
    };

    if (log->write_pos > log->buffer.size)
    {
        result.buf += ((log->write_pos + 1) & (log->buffer.size - 1));
        result.len = log->buffer.size - 1;
    }

    return result;
}

Cstr
cfLogCstring(CfLog *log)
{
    Usize offset = 0;

    if (log->write_pos > log->buffer.size)
    {
        offset = ((log->write_pos + 1) & (log->buffer.size - 1));
    }

    return (Char8 *)log->buffer.data + offset;
}

void
cfLogClear(CfLog *log)
{
    log->write_pos = 0;
}
