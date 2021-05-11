#ifndef FOUNDATION_STRINGS_H

#include "common.h"

#include "allocator.h"
#include "array.h"

#include <stdarg.h>
#include <stdio.h>

#if defined(__clang__)
#define CF_PRINTF_LIKE(fmt_argno, variadic_argno) \
    __attribute__((__format__(__printf__, fmt_argno + 1, variadic_argno + 1)))
#else
CF_PRINTF_LIKE(fmt_argno, variadic_argno)
#endif

// Compute the size of the string buffer (including the null terminator)
static inline usize
strSize(char const *str)
{
    CF_ASSERT_NOT_NULL(str);
    u32 size = 1;
    while (str[size - 1]) size++;
    return size;
}

// Compute the length of the string (ignoring the null terminator)
static inline usize
strLength(char const *str)
{
    return strSize(str) - 1;
}

/// Print formatted string on a buffer allocated by the given allocator
char *strPrintfAlloc(cfAllocator *alloc, usize *out_size, char const *fmt, ...);

/// Print formatted string on the given dynamic buffer
bool strPrintfBuffer(cfArray(char) * buffer, char const *fmt, ...);

/// Print formatted string on the given static buffer
bool strPrintf(char *buffer, usize buffer_size, char const *fmt, ...);

#define FOUNDATION_STRINGS_H
#endif
