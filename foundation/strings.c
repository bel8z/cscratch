#include "strings.h"

CF_PRINTF_LIKE(2, 3)
char *
strPrintfAlloc(cfAllocator *alloc, usize *out_size, char const *fmt, ...)
{
    va_list args, args_copy;

    va_start(args, fmt);
    va_copy(args_copy, args);

    i32 len = vsnprintf(NULL, 0, fmt, args_copy);
    if (len < 0) return NULL;

    usize size = (usize)len + 1;
    char *buf = cfAlloc(alloc, size);
    if (!buf) return NULL;

    vsnprintf(buf, size, fmt, args);

    va_end(args);

    *out_size = size;

    return buf;
}

CF_PRINTF_LIKE(1, 2)
bool
strPrintfBuffer(cfArray(char) * array, char const *fmt, ...)
{
    va_list args, args_copy;

    va_start(args, fmt);
    va_copy(args_copy, args);

    i32 len = vsnprintf(NULL, 0, fmt, args_copy);
    if (len < 0) return false;

    usize req_size = (usize)len + 1;
    usize arr_size = cfArraySize(*array);

    cfArrayResize(*array, req_size);

    vsnprintf(*array + arr_size, req_size, fmt, args);

    va_end(args);

    return true;
}

CF_PRINTF_LIKE(2, 3)
bool
strPrintf(char *buffer, usize buffer_size, char const *fmt, ...)
{
    va_list args, args_copy;

    va_start(args, fmt);
    va_copy(args_copy, args);

    i32 len = vsnprintf(NULL, 0, fmt, args_copy);
    if (len < 0) return false;

    usize req_size = (usize)len + 1;
    if (req_size < buffer_size) return false;

    vsnprintf(buffer, req_size, fmt, args);

    va_end(args);

    return true;
}
