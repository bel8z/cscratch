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
#define strFromC(str) \
    (Str) { .len = strLength(str), .buf = str }

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

/// Print formatted string on the given static buffer
/// This does not take a Str because it represents a string view more than a char buffer.
/// You can use a Str by explicitly calling strPrintf(str.buf, str.len, ...).
bool strPrintf(char *buffer, Usize buffer_size, char const *fmt, ...);

/// Print formatted string on the given dynamic buffer
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
