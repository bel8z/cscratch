#pragma once

/// String manipulation utilities
/// This is not an API header, include it in implementation files only

// TODO (Matteo): Improve API and usage

#include "core.h"

#define strValid(str) (!!(str).buf)

//----------------------//
//   C string helpers   //
//----------------------//

/// Build a string view from a C string
#define strFromCstr(cstr) \
    (Str) { .buf = (cstr), .len = strLength(cstr), }

/// Writes the string slice as a null-terminated C string on the given buffer
/// Returns the number of bytes written, including the null terminator, or the required buffer size
/// if NULL
Usize strToCstr(Str str, char *buffer, Usize size);

/// Compute the length of the C string (null terminator ignored)
static inline Usize
strLength(char const *str)
{
    CF_ASSERT_NOT_NULL(str);
    U32 size = 0;
    while (str[size]) size++;
    return size;
}

/// Compute the full size of the C string (null terminator included)
static inline Usize
strSize(char const *str)
{
    return strLength(str) + 1;
}

//-----------------------//
//   String formatting   //
//-----------------------//

#if CF_COMPILER_CLANG
#    define CF_PRINTF_LIKE(fmt_argno, variadic_argno) \
        __attribute__((__format__(__printf__, fmt_argno + 1, variadic_argno + 1)))
#else
#    define CF_PRINTF_LIKE(fmt_argno, variadic_argno)
#endif

/// Print formatted string on the given static buffer
/// This does not take a Str because it represents a string view more than a char buffer.
/// You can use a Str by explicitly calling strPrintf(str.buf, str.len, ...).
CF_PRINTF_LIKE(2, 3)
bool strPrintf(char *buffer, Usize buffer_size, char const *fmt, ...);

/// Print formatted string on the given dynamic buffer
CF_PRINTF_LIKE(1, 2)
bool strBufferPrintf(StrBuffer *buffer, char const *fmt, ...);

//------------------------------//
//   String (view) comparison   //
//------------------------------//

I32 strCompare(Str l, Str r);
bool strEqual(Str l, Str r);
I32 strCompareInsensitive(Str l, Str r);
bool strEqualInsensitive(Str l, Str r);

//------------------------------//
//   String (view) processing   //
//------------------------------//

bool strContains(Str str, char c);
Usize strFindFirst(Str haystack, Str needle);
Usize strFindLast(Str haystack, Str needle);
