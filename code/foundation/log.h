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

Cstr cfLogCstring(CfLog *log);

void cfLogClear(CfLog *log);

void cfLogAppend(CfLog *log, Str string);
void cfLogAppendC(CfLog *log, Cstr cstring);
void cfLogAppendF(CfLog *log, Cstr format, ...) CF_PRINTF_LIKE(1, 2);
