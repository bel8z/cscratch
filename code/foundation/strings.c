#include "strings.h"

#include "memory.h"

#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>

//----------------------//
//   C string helpers   //
//----------------------//

Usize
strToCstr(Str str, Char8 *buffer, Usize size)
{
    Usize len = cfMin(str.len, size);
    if (buffer)
    {
        memCopy(str.buf, buffer, len);
        buffer[len] = 0;
    }
    return len + 1;
}

//-----------------------//
//   String formatting   //
//-----------------------//

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
strBufferPrintf(StrBuffer *str, Cstr fmt, ...)
{
    va_list args;

    va_start(args, fmt);

    I32 len = strPrintfV(str->data, str->size, fmt, args);

    va_end(args);

    if (len < 0) return false;

    // TODO (Matteo): Should account for the null terminator?
    str->size = len;

    return true;
}

//-----------------------//
//   String comparison   //
//-----------------------//

I32
strCompare(Str l, Str r)
{
    return memCompare(l.buf, r.buf, cfMin(l.len, r.len));
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
    return (l.len == r.len && memMatch(l.buf, r.buf, l.len));
}

bool
strEqualInsensitive(Str l, Str r)
{
    // TODO (Matteo): replace with portable method
    return (l.len == r.len && !__strIComp(l.buf, r.buf, l.len));
}

//----------------------------//
//   String view processing   //
//----------------------------//

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

//-----------------------------//
//   Dynamic string building   //
//-----------------------------//

#define _sbValidate(sb) \
    (CF_ASSERT((sb) && (sb)->data && (sb)->size >= 1, "Invalid string builder state")) // NOLINT

static void
strBuilderResize(StrBuilder *sb, Usize new_size)
{
    if (sb->capacity < new_size)
    {
        Usize next_cap = cfMax(new_size, memGrowArrayCapacity(sb->capacity));
        sb->data = memRealloc(sb->alloc, sb->data, sb->capacity, next_cap);
        sb->capacity = next_cap;
    }

    sb->size = new_size;
}

void
strBuilderInit(StrBuilder *sb, MemAllocator alloc)
{
    CF_ASSERT_NOT_NULL(sb);
    sb->alloc = alloc;
    sb->capacity = 1;
    sb->data = memAlloc(alloc, sb->capacity);
    strBuilderClear(sb);
}

void
strBuilderInitFrom(StrBuilder *sb, MemAllocator alloc, Str str)
{
    CF_ASSERT_NOT_NULL(sb);

    sb->alloc = alloc;
    sb->capacity = sb->size = str.len + 1;
    sb->data = memAlloc(alloc, sb->capacity);

    memCopy(str.buf, sb->data, str.len);
    sb->data[str.len] = 0;
}

void
strBuilderInitWith(StrBuilder *sb, MemAllocator alloc, Usize cap)
{
    sb->alloc = alloc;
    sb->capacity = cfMin(1, cap);
    sb->data = memAlloc(alloc, sb->capacity);
    strBuilderClear(sb);
}

void
strBuilderShutdown(StrBuilder *sb)
{
    memFree(sb->alloc, sb->data, sb->capacity);
}

void
strBuilderClear(StrBuilder *sb)
{
    CF_ASSERT(sb->capacity >= 1, "Invalid string builder capacity");
    sb->size = 1;
    sb->data[0] = 0;
}

void
strBuilderAppend(StrBuilder *sb, Str what)
{
    _sbValidate(sb);

    // Write over the previous null terminator
    Usize nul_pos = sb->size - 1;

    strBuilderResize(sb, sb->size + what.len);

    memCopy(what.buf, sb->data + nul_pos, what.len);

    // Null terminate again
    sb->data[sb->size - 1] = 0;
}

bool
strBuilderPrintf(StrBuilder *sb, Cstr fmt, ...)
{
    _sbValidate(sb);

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

    // Keep room for the null terminator
    strBuilderResize(sb, len + 1);

    vsnprintf(sb->data, sb->size, fmt, args); // NOLINT
    va_end(args);

    CF_ASSERT(sb->data && sb->data[sb->size - 1] == 0, "Missing null terminator");

    return true;
}

bool
strBuilderAppendf(StrBuilder *sb, Cstr fmt, ...)
{
    _sbValidate(sb);

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

    // Write over the previous null terminator
    Usize nul_pos = sb->size - 1;

    strBuilderResize(sb, sb->size + len);
    CF_ASSERT(sb->size >= len + 1, "Buffer not extended correctly");

    vsnprintf(sb->data + nul_pos, len + 1, fmt, args); // NOLINT
    va_end(args);

    // Null terminate again
    sb->data[sb->size - 1] = 0;

    return true;
}

Str
strBuilderView(StrBuilder *sb)
{
    _sbValidate(sb);
    return (Str){.buf = sb->data, .len = sb->size - 1};
}

Cstr
strBuilderCstr(StrBuilder *sb)
{
    _sbValidate(sb);
    return sb->data;
}
