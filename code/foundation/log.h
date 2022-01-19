#pragma once

#include "core.h"

typedef struct CfVirtualMemory CfVirtualMemory;

typedef struct CfLog
{
    void *os_handle;
    void *buffer;
    Usize size;
    Usize write_pos;
} CfLog;

CF_API CfLog cfLogCreate(CfVirtualMemory *vm, Usize buffer_size);
CF_API void cfLogDestroy(CfLog *log, CfVirtualMemory *vm);

CF_API Str cfLogString(CfLog *log);
CF_API Cstr cfLogCstring(CfLog *log);

CF_API void cfLogClear(CfLog *log);

CF_API void cfLogAppend(CfLog *log, Str string);
CF_API void cfLogAppendC(CfLog *log, Cstr cstring);
CF_API void cfLogAppendF(CfLog *log, Cstr format, ...) CF_PRINTF_LIKE(1);
