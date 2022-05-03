#pragma once

//------------------------------------------------------------------------------

/// String manipulation utilities
/// This is not an API header, include it in implementation files only

// TODO (Matteo): Improve API and usage

//------------------------------------------------------------------------------

#include "core.h"

//----------------------//
//   C string helpers   //
//----------------------//

/// Compute the length of the C string (null terminator ignored)
CF_INLINE_API Usize strLength(Cstr cstr);

/// Compute the full size of the C string (null terminator included)
CF_INLINE_API Usize strSize(Cstr cstr);

/// Build a string view from a C string
CF_INLINE_API Str strFromCstr(Cstr cstr);

/// Writes the string slice as a null-terminated C string on the given buffer
/// Returns the number of bytes written, including the null terminator, or the required buffer
/// size if NULL
CF_INLINE_API Usize strToCstr(Str str, Char8 *buffer, Usize size);

CF_INLINE_API void strBufferInit(StrBuffer *buffer);

//-----------------------//
//   String formatting   //
//-----------------------//

/// Print formatted string on the given static buffer
/// This does not take a Str because it represents a string view more than a char buffer.
/// You can use a Str by explicitly calling strPrintV(str.buf, str.len, ...).
CF_API Isize strPrintV(Char8 *buffer, Usize buffer_size, Cstr fmt, va_list args) CF_VPRINTF_LIKE(2);

/// Print formatted string on the given static buffer
/// This does not take a Str because it represents a string view more than a char buffer.
/// You can use a Str by explicitly calling strPrint(str.buf, str.len, ...).
CF_API ErrorCode32 strPrint(Char8 *buffer, Usize buffer_size, Cstr fmt, ...) CF_PRINTF_LIKE(2);

/// Print formatted string on the given dynamic buffer
CF_API ErrorCode32 strBufferPrint(StrBuffer *buf, Cstr fmt, ...) CF_PRINTF_LIKE(1);

CF_API ErrorCode32 strBufferAppendStr(StrBuffer *buf, Str what);
CF_API ErrorCode32 strBufferAppend(StrBuffer *buf, Cstr fmt, ...) CF_PRINTF_LIKE(1);
CF_API ErrorCode32 strBufferAppendV(StrBuffer *buf, Cstr fmt, va_list args) CF_VPRINTF_LIKE(1);

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

CF_API ErrorCode32 strBuilderInit(StrBuilder *sb, MemAllocator alloc);
CF_API ErrorCode32 strBuilderInitFrom(StrBuilder *sb, MemAllocator alloc, Str str);
CF_API ErrorCode32 strBuilderInitWith(StrBuilder *sb, MemAllocator alloc, Usize cap);
CF_API void strBuilderShutdown(StrBuilder *sb);

CF_API void strBuilderClear(StrBuilder *sb);
CF_API ErrorCode32 strBuilderAppendStr(StrBuilder *sb, Str what);
CF_API ErrorCode32 strBuilderAppend(StrBuilder *sb, Cstr fmt, ...) CF_PRINTF_LIKE(1);
CF_API ErrorCode32 strBuilderAppendV(StrBuilder *sb, Cstr fmt, va_list args) CF_VPRINTF_LIKE(1);
CF_API ErrorCode32 strBuilderPrint(StrBuilder *sb, Cstr fmt, ...) CF_PRINTF_LIKE(1);
CF_API ErrorCode32 strBuilderPrintV(StrBuilder *sb, Cstr fmt, va_list args) CF_VPRINTF_LIKE(1);

CF_API Str strBuilderView(StrBuilder *sb);
CF_API Cstr strBuilderCstr(StrBuilder *sb);

//------------------------------------------------------------------------------
