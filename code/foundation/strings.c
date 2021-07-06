#include "strings.h"

#include "array.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#if CF_COMPILER_CLANG
#    define CF_PRINTF_LIKE(fmt_argno, variadic_argno) \
        __attribute__((__format__(__printf__, fmt_argno + 1, variadic_argno + 1)))
#else
#    define CF_PRINTF_LIKE(fmt_argno, variadic_argno)
#endif

Usize
strToCstr(Str str, char *buffer, Usize size)
{
    Usize len = cfMin(str.len, size);
    if (buffer)
    {
        cfMemCopy(str.buf, buffer, len);
        buffer[len] = 0;
    }
    return len + 1;
}

CF_PRINTF_LIKE(1, 2)
bool
strBufferPrintf(StrBuffer *array, char const *fmt, ...)
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
strCompare(Str l, Str r)
{
    return cfMemCompare(l.buf, r.buf, cfMin(l.len, r.len));
}

static inline I32
__strIComp(char const *l, char const *r, Usize size)
{
    // TODO (Matteo): replace with portable method
    return _strnicmp(l, r, size);
}

I32
strCompareInsensitive(Str l, Str r)
{
    return __strIComp(l.buf, r.buf, cfMin(l.len, r.len));
}

bool
strEqual(Str l, Str r)
{
    return (l.len == r.len && cfMemMatch(l.buf, r.buf, l.len));
}

bool
strEqualInsensitive(Str l, Str r)
{
    // TODO (Matteo): replace with portable method
    return (l.len == r.len && !__strIComp(l.buf, r.buf, l.len));
}

Usize
strFindFirst(Str haystack, Str needle)
{
    for (Usize h = 0; h < haystack.len; ++h)
    {
        for (Usize n = 0; n < needle.len; ++n)
        {
            if (haystack.buf[h] == needle.buf[n])
            {
                return h;
            }
        }
    }

    return USIZE_MAX;
}

Usize
strFindLast(Str haystack, Str needle)
{
    for (Usize h = haystack.len; h > 0; --h)
    {
        for (Usize n = needle.len; n > 0; --n)
        {
            if (haystack.buf[h - 1] == needle.buf[n - 1])
            {
                return h - 1;
            }
        }
    }

    return USIZE_MAX;
}

bool
strContains(Str str, char c)
{
    for (Usize i = 0; i < str.len; ++i)
    {
        if (str.buf[i] == c) return true;
    }

    return false;
}
