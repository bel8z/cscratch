#include "strings.h"

#include "allocator.h"
#include "array.h"

#include <stdarg.h>
#include <stdio.h>

#if defined(__clang__)
#define CF_PRINTF_LIKE(fmt_argno, variadic_argno) \
    __attribute__((__format__(__printf__, fmt_argno + 1, variadic_argno + 1)))
#else
#define CF_PRINTF_LIKE(fmt_argno, variadic_argno)
#endif

CF_PRINTF_LIKE(2, 3)
char *
strPrintfAlloc(cfAllocator *alloc, usize *out_size, char const *fmt, ...)
{
    va_list args, args_copy;

    va_start(args, fmt);
    va_copy(args_copy, args);

    i32 len = vsnprintf(NULL, 0, fmt, args_copy); // NOLINT

    va_end(args_copy);

    if (len < 0)
    {
        va_end(args);
        return NULL;
    };

    usize size = (usize)len + 1;
    char *buf = cfAlloc(alloc, size);
    if (buf)
    {
        vsnprintf(buf, size, fmt, args); // NOLINT
        *out_size = size;
    };

    va_end(args);

    return buf;
}

CF_PRINTF_LIKE(1, 2)
bool
strPrintfBuffer(cfArray(char) * array, char const *fmt, ...)
{
    va_list args, args_copy;

    va_start(args, fmt);
    va_copy(args_copy, args);

    i32 len = vsnprintf(NULL, 0, fmt, args_copy); // NOLINT

    va_end(args_copy);

    if (len < 0)
    {
        va_end(args);
        return false;
    };

    usize req_size = (usize)len + 1;
    usize arr_size = cfArraySize(*array);

    cfArrayResize(*array, req_size);

    vsnprintf(*array + arr_size, req_size, fmt, args); // NOLINT

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

    i32 len = vsnprintf(NULL, 0, fmt, args_copy); // NOLINT

    va_end(args_copy);

    usize req_size = (usize)len + 1;

    if (len < 0 || req_size > buffer_size)
    {
        va_end(args);
        return false;
    }

    vsnprintf(buffer, req_size, fmt, args); // NOLINT

    va_end(args);

    return true;
}
