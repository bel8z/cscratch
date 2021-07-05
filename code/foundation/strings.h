#pragma once

#include "core.h"

// Compute the size of the string buffer (including the null terminator)
static inline Usize
strSize(char const *str)
{
    CF_ASSERT_NOT_NULL(str);
    U32 size = 1;
    while (str[size - 1]) size++;
    return size;
}

// Compute the length of the string (ignoring the null terminator)
static inline Usize
strLength(char const *str)
{
    return strSize(str) - 1;
}

/// Print formatted string on a buffer allocated by the given allocator
char *strPrintfAlloc(cfAllocator alloc, Usize *out_size, char const *fmt, ...);

/// Print formatted string on the given dynamic buffer
bool strPrintfBuffer(StrBuffer *buffer, char const *fmt, ...);

/// Print formatted string on the given static buffer
bool strPrintf(char *buffer, Usize buffer_size, char const *fmt, ...);

I32 strCompare(char const *l, char const *r);
I32 strCompareInsensitive(char const *l, char const *r);
bool strEqual(char const *l, char const *r);
bool strEqualInsensitive(char const *l, char const *r);
