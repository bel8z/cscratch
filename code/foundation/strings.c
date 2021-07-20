#include "strings.h"

#include "array.h"

#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>

Usize
strToCstr(Str str, Char8 *buffer, Usize size)
{
    Usize len = cfMin(str.len, size);
    if (buffer)
    {
        cfMemCopy(str.buf, buffer, len);
        buffer[len] = 0;
    }
    return len + 1;
}

bool
strBufferPrintf(StrBuffer *array, Cstr fmt, ...)
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

// NOTE (Matteo): This returns the length of the written string, ignoring the null terminator
static I32
strPrintfV(Char8 *buffer, Usize buffer_size, Cstr fmt, va_list args)
{
    va_list args_copy;

    va_copy(args_copy, args);

    I32 len = vsnprintf(NULL, 0, fmt, args_copy); // NOLINT

    va_end(args_copy);

    if (len < 0 || (len + 1) > buffer_size) return -1;

    vsnprintf(buffer, len + 1, fmt, args); // NOLINT

    return len;
}

bool
strPrintf(Char8 *buffer, Usize buffer_size, Cstr fmt, ...)
{
    va_list args;

    va_start(args, fmt);

    I32 len = strPrintfV(buffer, buffer_size, fmt, args);

    va_end(args);

    return (len >= 0);
}

bool
stackStrPrintf(StackStr *str, Cstr fmt, ...)
{
    va_list args;

    va_start(args, fmt);

    I32 len = strPrintfV(str->buf, str->len, fmt, args);

    va_end(args);

    if (len < 0) return false;

    // TODO (Matteo): Should account for the null terminator?
    str->len = len;

    return true;
}

I32
strCompare(Str l, Str r)
{
    return cfMemCompare(l.buf, r.buf, cfMin(l.len, r.len));
}

static inline I32
__strIComp(Cstr l, Cstr r, Usize size)
{
    I32 diff = 0;

    for (Usize i = 0; i < size && !diff; ++i)
    {
        diff = tolower(l[i]) - tolower(r[i]);
    }

    return diff;
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
strContains(Str str, Char8 c)
{
    for (Usize i = 0; i < str.len; ++i)
    {
        if (str.buf[i] == c) return true;
    }

    return false;
}
