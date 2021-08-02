#include "log.h"
#include "strings.h"

#include <stdarg.h>

CfLog
cfLogCreate(CfVirtualMemory *vm, Usize buffer_size)
{
    return (CfLog){.buffer = cfVmMirrorAllocate(vm, buffer_size)};
}

void
cfLogDestroy(CfLog *log, CfVirtualMemory *vm)
{
    cfVmMirrorFree(vm, &log->buffer);
}

void
cfLogAppend(CfLog *log, Str string)
{

    U8 *ptr = log->buffer.data + (log->write_pos & (log->buffer.size - 1));
    cfMemCopy(string.buf, ptr, string.len);
    log->write_pos += string.len;
}

void
cfLogAppendf(CfLog *log, Cstr format, ...)
{
    char *ptr = (char *)log->buffer.data + (log->write_pos & (log->buffer.size - 1));

    va_list args, copy;
    va_start(args, format);

    va_copy(copy, args);
    va_start(copy, format);
    Usize len = (Usize)vsnprintf(NULL, 0, format, copy);
    va_end(copy);

    CF_ASSERT(len < log->buffer.size, "What?");

    vsnprintf(ptr, len + 1, format, args);
    va_end(args);

    log->write_pos += len;
}

Cstr
cfLogCstring(CfLog *log)
{
    Usize offset = 0;

    if (log->write_pos > log->buffer.size)
    {
        offset = ((log->write_pos + 1) & (log->buffer.size - 1));
    }

    return (char *)log->buffer.data + offset;
}
