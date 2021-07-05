#include "strings.h"

#include "array.h"

#include <stdarg.h>
#include <stdio.h>

#if CF_COMPILER_CLANG
#    define CF_PRINTF_LIKE(fmt_argno, variadic_argno) \
        __attribute__((__format__(__printf__, fmt_argno + 1, variadic_argno + 1)))
#else
#    define CF_PRINTF_LIKE(fmt_argno, variadic_argno)
#endif

CF_PRINTF_LIKE(2, 3)
char *
strPrintfAlloc(cfAllocator alloc, Usize *out_size, char const *fmt, ...)
{
    va_list args, args_copy;

    va_start(args, fmt);
    va_copy(args_copy, args);

    I32 len = vsnprintf(NULL, 0, fmt, args_copy); // NOLINT

    va_end(args_copy);

    if (len < 0)
    {
        va_end(args);
        return NULL;
    };

    Usize size = (Usize)len + 1;
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
strPrintfBuffer(StrBuffer *array, char const *fmt, ...)
{
    va_list args, args_copy;

    va_start(args, fmt);
    va_copy(args_copy, args);

    I32 len = vsnprintf(NULL, 0, fmt, args_copy); // NOLINT

    va_end(args_copy);

    if (len < 0)
    {
        va_end(args);
        return false;
    };

    Usize req_size = (Usize)len + 1;
    Usize arr_size = array->len;

    cfArrayResize(array, req_size);

    vsnprintf(array->buf + arr_size, req_size, fmt, args); // NOLINT

    va_end(args);

    return true;
}

CF_PRINTF_LIKE(2, 3)
bool
strPrintf(char *buffer, Usize buffer_size, char const *fmt, ...)
{
    va_list args, args_copy;

    va_start(args, fmt);
    va_copy(args_copy, args);

    I32 len = vsnprintf(NULL, 0, fmt, args_copy); // NOLINT

    va_end(args_copy);

    Usize req_size = (Usize)len + 1;

    if (len < 0 || req_size > buffer_size)
    {
        va_end(args);
        return false;
    }

    vsnprintf(buffer, req_size, fmt, args); // NOLINT

    va_end(args);

    return true;
}

I32
strCompare(char const *l, char const *r)
{
    return strcmp(l, r);
}

I32
strCompareInsensitive(char const *l, char const *r)
{
    // TODO (Matteo): replace with portable method
    return _strcmpi(l, r);
}

bool
strEqual(char const *l, char const *r)
{
    return !strCompare(l, r);
}

bool
strEqualInsensitive(char const *l, char const *r)
{
    return !strCompareInsensitive(l, r);
}
