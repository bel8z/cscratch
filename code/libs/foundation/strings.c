#include "strings.h"

#include "error.h"
#include "memory.h"

#include "mem_buffer.inl"

#include <ctype.h>
#include <stdio.h>

//----------------------//
//   C string helpers   //
//----------------------//

Size
strLength(Cstr cstr)
{
    CF_ASSERT_NOT_NULL(cstr);
    U32 size = 0;
    while (cstr[size]) size++;
    return size;
}

Size
strSize(Cstr cstr)
{
    return strLength(cstr) + 1;
}

Str
strFromCstr(Cstr cstr)
{
    return (Str){.ptr = (cstr), .len = strLength(cstr)};
}

Size
strToCstr(Str str, Char8 *buffer, Size size)
{
    Size len = cfMin(str.len, size);
    if (buffer)
    {
        memCopy(str.ptr, buffer, len);
        buffer[len] = 0;
    }
    return len + 1;
}

void
strBufferInit(StrBuffer *buffer)
{
    buffer->str.len = 0;
    buffer->str.ptr = (Char8 const *)buffer->data;
}

//-----------------------//
//   String formatting   //
//-----------------------//

// NOTE (Matteo): This returns the length of the written string, ignoring the null terminator
Offset
strPrintV(Char8 *buffer, Size buffer_size, Cstr fmt, va_list args)
{
    va_list args_copy;

    va_copy(args_copy, args);

    Offset len = vsnprintf(NULL, 0, fmt, args_copy); // NOLINT

    va_end(args_copy);

    Size size = (Size)(len + 1);
    if (len < 0 || size > buffer_size) return -1;

    if (buffer) vsnprintf(buffer, size, fmt, args); // NOLINT

    return len;
}

Offset
strPrint(Char8 *buffer, Size buffer_size, Cstr fmt, ...)
{
    va_list args;

    va_start(args, fmt);

    Offset len = strPrintV(buffer, buffer_size, fmt, args);

    va_end(args);

    return len;
}

ErrorCode32
strBufferPrint(StrBuffer *buf, Cstr fmt, ...)
{
    va_list args;

    va_start(args, fmt);

    Offset len = strPrintV(buf->data, buf->str.len, fmt, args);

    va_end(args);

    if (len < 0) return Error_BufferFull;

    // TODO (Matteo): Should account for the null terminator?
    buf->str.len = (Size)len;

    return Error_None;
}

ErrorCode32
strBufferAppendStr(StrBuffer *buf, Str what)
{
    Size avail = CF_ARRAY_SIZE(buf->data) - buf->str.len;
    if (avail < what.len) return Error_BufferFull;

    memCopy(what.ptr, buf->data + buf->str.len, what.len);
    buf->str.len += what.len;

    return Error_None;
}

//-----------------------//
//   String comparison   //
//-----------------------//

static inline I32
str_compareInsensitive(Cstr l, Cstr r, Size size)
{
    I32 diff = 0;

    for (Size i = 0; i < size && !diff; ++i)
    {
        diff = tolower(l[i]) - tolower(r[i]);
    }

    return diff;
}

I32
strCompareInsensitive(Str l, Str r)
{
    return str_compareInsensitive(l.ptr, r.ptr, cfMin(l.len, r.len));
}

bool
strEqualInsensitive(Str l, Str r)
{
    return (l.len == r.len && !str_compareInsensitive(l.ptr, r.ptr, l.len));
}

//----------------------------//
//   String view processing   //
//----------------------------//

Size
strFindFirst(Str haystack, Str needle)
{
    for (Size h = 0; h < haystack.len; ++h)
    {
        for (Size n = 0; n < needle.len; ++n)
        {
            if (haystack.ptr[h] == needle.ptr[n])
            {
                return h;
            }
        }
    }

    return SIZE_MAX;
}

Size
strFindLast(Str haystack, Str needle)
{
    for (Size h = haystack.len; h > 0; --h)
    {
        for (Size n = needle.len; n > 0; --n)
        {
            if (haystack.ptr[h - 1] == needle.ptr[n - 1])
            {
                return h - 1;
            }
        }
    }

    return SIZE_MAX;
}

bool
strContains(Str str, Char8 c)
{
    for (Size i = 0; i < str.len; ++i)
    {
        if (str.ptr[i] == c) return true;
    }

    return false;
}

//-----------------------------//
//   Dynamic string building   //
//-----------------------------//

static inline void
strBuilderValidate(StrBuilder *sb)
{
    (CF_ASSERT((sb) && (sb)->ptr && (sb)->len >= 1, "Invalid string builder state"));
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
        memCopy(str.ptr, sb->ptr, str.len);
        sb->ptr[str.len] = 0;
    }
    return err;
}

ErrorCode32
strBuilderInitWith(StrBuilder *sb, MemAllocator alloc, Size cap)
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
    CF_ASSERT_NOT_NULL(sb->ptr);
    CF_ASSERT(sb->cap >= 1, "Invalid string builder capacity");
    sb->len = 1;
    sb->ptr[0] = 0;
}

ErrorCode32
strBuilderAppendStr(StrBuilder *sb, Str what)
{
    strBuilderValidate(sb);

    // Write over the previous null terminator
    Size nul_pos = sb->len - 1;

    ErrorCode32 err = memBufferResizeAlloc(sb, sb->len + what.len, sb->alloc);
    if (err) return err;

    memCopy(what.ptr, sb->ptr + nul_pos, what.len);

    // Null terminate again
    sb->ptr[sb->len - 1] = 0;

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

    Offset len = vsnprintf(NULL, 0, fmt, args_copy); // NOLINT

    va_end(args_copy);

    if (len < 0) return Error_Reserved; // TODO (Matteo): Better diagnostics

    // Keep room for the null terminator
    ErrorCode32 err = memBufferResizeAlloc(sb, len + 1, sb->alloc);
    if (err) return err;

    vsnprintf(sb->ptr, sb->len, fmt, args); // NOLINT

    CF_ASSERT(sb->ptr && sb->ptr[sb->len - 1] == 0, "Missing null terminator");

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

    Offset len = vsnprintf(NULL, 0, fmt, args_copy); // NOLINT

    va_end(args_copy);

    if (len < 0) return Error_Reserved; // TODO (Matteo): Better diagnostics

    // Write over the previous null terminator
    Size nul_pos = sb->len - 1;

    ErrorCode32 err = memBufferResizeAlloc(sb, sb->len + (Size)len, sb->alloc);
    if (err) return err;

    Size size = (Size)(len + 1);
    CF_ASSERT(sb->len >= size, "Buffer not extended correctly");

    vsnprintf(sb->ptr + nul_pos, size, fmt, args); // NOLINT

    // Null terminate again
    sb->ptr[sb->len - 1] = 0;

    return Error_None;
}

Str
strBuilderView(StrBuilder *sb)
{
    strBuilderValidate(sb);
    return (Str){.ptr = sb->ptr, .len = sb->len - 1};
}

Cstr
strBuilderCstr(StrBuilder *sb)
{
    strBuilderValidate(sb);
    return sb->ptr;
}
