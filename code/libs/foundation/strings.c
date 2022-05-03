#include "strings.h"

#include "error.h"
#include "memory.h"

#include "mem_buffer.inl"

#include <ctype.h>
#include <stdio.h>

//----------------------//
//   C string helpers   //
//----------------------//

Usize
strLength(Cstr cstr)
{
    CF_ASSERT_NOT_NULL(cstr);
    U32 size = 0;
    while (cstr[size]) size++;
    return size;
}

Usize
strSize(Cstr cstr)
{
    return strLength(cstr) + 1;
}

Str
strFromCstr(Cstr cstr)
{
    return (Str){.buf = (cstr), .len = strLength(cstr)};
}

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

void
strBufferInit(StrBuffer *buffer)
{
    buffer->str.len = 0;
    buffer->str.buf = (Char8 const *)buffer->data;
}

//-----------------------//
//   String formatting   //
//-----------------------//

// NOTE (Matteo): This returns the length of the written string, ignoring the null terminator
Isize
strPrintV(Char8 *buffer, Usize buffer_size, Cstr fmt, va_list args)
{
    va_list args_copy;

    va_copy(args_copy, args);

    Isize len = vsnprintf(NULL, 0, fmt, args_copy); // NOLINT

    va_end(args_copy);

    if (len < 0 || (len + 1) > buffer_size) return -1;

    if (buffer) vsnprintf(buffer, len + 1, fmt, args); // NOLINT

    return len;
}

ErrorCode32
strPrint(Char8 *buffer, Usize buffer_size, Cstr fmt, ...)
{
    va_list args;

    va_start(args, fmt);

    Isize len = strPrintV(buffer, buffer_size, fmt, args);

    va_end(args);

    return (len >= 0) ? Error_None : Error_BufferFull;
}

ErrorCode32
strBufferPrint(StrBuffer *buf, Cstr fmt, ...)
{
    va_list args;

    va_start(args, fmt);

    Isize len = strPrintV(buf->data, buf->str.len, fmt, args);

    va_end(args);

    if (len < 0) return Error_BufferFull;

    // TODO (Matteo): Should account for the null terminator?
    buf->str.len = len;

    return Error_None;
}

ErrorCode32
strBufferAppendStr(StrBuffer *buf, Str what)
{
    Usize avail = CF_ARRAY_SIZE(buf->data) - buf->str.len;
    if (avail < what.len) return Error_BufferFull;

    memCopy(what.buf, buf->data + buf->str.len, what.len);
    buf->str.len += what.len;

    return Error_None;
}

ErrorCode32
strBufferAppend(StrBuffer *buf, Cstr fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    ErrorCode32 result = strBufferAppendV(buf, fmt, args);
    va_end(args);
    return result;
}

ErrorCode32
strBufferAppendV(StrBuffer *buf, Cstr fmt, va_list args)
{
    va_list args_copy;
    va_copy(args_copy, args);
    Isize len = vsnprintf(NULL, 0, fmt, args_copy); // NOLINT
    va_end(args_copy);

    if (len < 0) return Error_Reserved; // TODO (Matteo): Better diagnostics

    Usize avail = CF_ARRAY_SIZE(buf->data) - buf->str.len;
    if (avail < len) return Error_BufferFull;

    vsnprintf(buf->data + buf->str.len, avail, fmt, args); // NOLINT
    buf->str.len += len;

    return Error_None;
}

//-----------------------//
//   String comparison   //
//-----------------------//

I32
strCompare(Str l, Str r)
{
    return memCompare(l.buf, r.buf, cfMin(l.len, r.len));
}

CF_INTERNAL inline I32
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

CF_INTERNAL inline void
strBuilderValidate(StrBuilder *sb)
{
    (CF_ASSERT((sb) && (sb)->data && (sb)->size >= 1, "Invalid string builder state"));
}

ErrorCode32
strBuilderInit(StrBuilder *sb, MemAllocator alloc)
{
    CF_ASSERT_NOT_NULL(sb);

    sb->alloc = alloc;
    ErrorCode32 err = memBufferEnsure(sb, 1, alloc);
    if (!err) strBuilderClear(sb);
    return err;
}

ErrorCode32
strBuilderInitFrom(StrBuilder *sb, MemAllocator alloc, Str str)
{
    CF_ASSERT_NOT_NULL(sb);

    sb->alloc = alloc;
    ErrorCode32 err = memBufferEnsure(sb, str.len + 1, alloc);
    if (!err)
    {
        memCopy(str.buf, sb->data, str.len);
        sb->data[str.len] = 0;
    }
    return err;
}

ErrorCode32
strBuilderInitWith(StrBuilder *sb, MemAllocator alloc, Usize cap)
{
    CF_ASSERT_NOT_NULL(sb);

    sb->alloc = alloc;
    ErrorCode32 err = memBufferEnsure(sb, cfMin(1, cap), alloc);
    if (!err) strBuilderClear(sb);
    return err;
}

void
strBuilderShutdown(StrBuilder *sb)
{
    CF_ASSERT_NOT_NULL(sb);
    memBufferFree(sb, sb->alloc);
}

void
strBuilderClear(StrBuilder *sb)
{
    CF_ASSERT_NOT_NULL(sb);
    CF_ASSERT_NOT_NULL(sb->data);
    CF_ASSERT(sb->capacity >= 1, "Invalid string builder capacity");
    sb->size = 1;
    sb->data[0] = 0;
}

ErrorCode32
strBuilderAppendStr(StrBuilder *sb, Str what)
{
    strBuilderValidate(sb);

    // Write over the previous null terminator
    Usize nul_pos = sb->size - 1;

    ErrorCode32 err = memBufferResizeAlloc(sb, sb->size + what.len, sb->alloc);
    if (err) return err;

    memCopy(what.buf, sb->data + nul_pos, what.len);

    // Null terminate again
    sb->data[sb->size - 1] = 0;

    return Error_None;
}

ErrorCode32
strBuilderPrint(StrBuilder *sb, Cstr fmt, ...)
{
    strBuilderValidate(sb);

    va_list args;
    va_start(args, fmt);
    ErrorCode32 result = strBuilderPrintV(sb, fmt, args);
    va_end(args);

    return result;
}

ErrorCode32
strBuilderPrintV(StrBuilder *sb, Cstr fmt, va_list args)
{
    strBuilderValidate(sb);

    va_list args_copy;
    va_copy(args_copy, args);

    Isize len = vsnprintf(NULL, 0, fmt, args_copy); // NOLINT

    va_end(args_copy);

    if (len < 0) return Error_Reserved; // TODO (Matteo): Better diagnostics

    // Keep room for the null terminator
    ErrorCode32 err = memBufferResizeAlloc(sb, len + 1, sb->alloc);
    if (err) return err;

    vsnprintf(sb->data, sb->size, fmt, args); // NOLINT

    CF_ASSERT(sb->data && sb->data[sb->size - 1] == 0, "Missing null terminator");

    return Error_None;
}

ErrorCode32
strBuilderAppend(StrBuilder *sb, Cstr fmt, ...)
{
    strBuilderValidate(sb);

    va_list args;
    va_start(args, fmt);
    ErrorCode32 result = strBuilderAppendV(sb, fmt, args);
    va_end(args);

    return result;
}

ErrorCode32
strBuilderAppendV(StrBuilder *sb, Cstr fmt, va_list args)
{
    strBuilderValidate(sb);

    va_list args_copy;
    va_copy(args_copy, args);

    Isize len = vsnprintf(NULL, 0, fmt, args_copy); // NOLINT

    va_end(args_copy);

    if (len < 0) return Error_Reserved; // TODO (Matteo): Better diagnostics

    // Write over the previous null terminator
    Usize nul_pos = sb->size - 1;

    ErrorCode32 err = memBufferResizeAlloc(sb, sb->size + len, sb->alloc);
    if (err) return err;

    CF_ASSERT(sb->size >= len + 1, "Buffer not extended correctly");

    vsnprintf(sb->data + nul_pos, len + 1, fmt, args); // NOLINT

    // Null terminate again
    sb->data[sb->size - 1] = 0;

    return Error_None;
}

Str
strBuilderView(StrBuilder *sb)
{
    strBuilderValidate(sb);
    return (Str){.buf = sb->data, .len = sb->size - 1};
}

Cstr
strBuilderCstr(StrBuilder *sb)
{
    strBuilderValidate(sb);
    return sb->data;
}
