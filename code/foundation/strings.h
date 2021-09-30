#pragma once

//------------------------------------------------------------------------------

/// String manipulation utilities
/// This is not an API header, include it in implementation files only

// TODO (Matteo): Improve API and usage

//------------------------------------------------------------------------------

#include "core.h"
#include "error.h"

#define strValid(str) (!!(str).buf)
#define strEnd(str) ((str).buf + (str).len)

//----------------------//
//   C string helpers   //
//----------------------//

/// Build a string view from a string literal (static C string)
#define strLiteral(lit) \
    (Str) { .buf = (lit), .len = CF_ARRAY_SIZE(lit) - 1, }

/// Compute the length of the C string (null terminator ignored)
static inline Usize
strLength(Cstr cstr)
{
    CF_ASSERT_NOT_NULL(cstr);
    U32 size = 0;
    while (cstr[size]) size++;
    return size;
}

/// Compute the full size of the C string (null terminator included)
static inline Usize
strSize(Cstr cstr)
{
    return strLength(cstr) + 1;
}

/// Build a string view from a C string
static inline Str
strFromCstr(Cstr cstr)
{
    return (Str){.buf = (cstr), .len = strLength(cstr)};
}

/// Writes the string slice as a null-terminated C string on the given buffer
/// Returns the number of bytes written, including the null terminator, or the required buffer
/// size if NULL
CF_API Usize strToCstr(Str str, Char8 *buffer, Usize size);

//-----------------------//
//   String formatting   //
//-----------------------//

/// Print formatted string on the given static buffer
/// This does not take a Str because it represents a string view more than a char buffer.
/// You can use a Str by explicitly calling strPrintf(str.buf, str.len, ...).
CF_API bool strPrintf(Char8 *buffer, Usize buffer_size, Cstr fmt, ...) CF_PRINTF_LIKE(2, 3);

/// Print formatted string on the given dynamic buffer
CF_API bool strBufferPrintf(StrBuffer *str, Cstr fmt, ...) CF_PRINTF_LIKE(1, 2);

//------------------------------//
//   String (view) comparison   //
//------------------------------//

CF_API I32 strCompare(Str l, Str r);
CF_API bool strEqual(Str l, Str r);
CF_API I32 strCompareInsensitive(Str l, Str r);
CF_API bool strEqualInsensitive(Str l, Str r);

//------------------------------//
//   String (view) processing   //
//------------------------------//

CF_API bool strContains(Str str, Char8 c);
CF_API Usize strFindFirst(Str haystack, Str needle);
CF_API Usize strFindLast(Str haystack, Str needle);

//-----------------------------//
//   Dynamic string building   //
//-----------------------------//

CF_API void strBuilderInit(StrBuilder *sb, MemAllocator alloc);
CF_API void strBuilderInitFrom(StrBuilder *sb, MemAllocator alloc, Str str);
CF_API void strBuilderInitWith(StrBuilder *sb, MemAllocator alloc, Usize cap);
CF_API void strBuilderShutdown(StrBuilder *sb);

CF_API void strBuilderAppend(StrBuilder *sb, Str what);
CF_API bool strBuilderAppendf(StrBuilder *sb, Cstr fmt, ...) CF_PRINTF_LIKE(1, 2);
CF_API bool strBuilderPrintf(StrBuilder *sb, Cstr fmt, ...) CF_PRINTF_LIKE(1, 2);

CF_API Str strBuilderView(StrBuilder *sb);
CF_API Cstr strBuilderCstr(StrBuilder *sb);

//------------------------------------------------------------------------------
