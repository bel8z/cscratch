#pragma once

#include "core.h"
#include "memory.h"

typedef struct CfLog
{
    CfMirrorBuffer buffer;
    Usize write_pos;
} CfLog;

CfLog cfLogCreate(CfVirtualMemory *vm, Usize buffer_size);
void cfLogDestroy(CfLog *log, CfVirtualMemory *vm);

inline Usize
cfLogSize(CfLog *log)
{
    return log->buffer.size;
}

Cstr cfLogCstring(CfLog *log);

void cfLogAppend(CfLog *log, Str string);
void cfLogAppendf(CfLog *log, Cstr format, ...) CF_PRINTF_LIKE(1, 2);
