// Attempt to include windows headers in a clean way

#pragma once

#pragma warning(push)
#pragma warning(disable : 5105)

#if !defined(NOMINMAX)
#    define NOMINMAX 1
#endif
#if !defined(VC_EXTRALEAN)
#    define VC_EXTRALEAN 1
#endif
#if !defined(WIN32_LEAN_AND_MEAN)
#    define WIN32_LEAN_AND_MEAN 1
#endif
#include <Windows.h>
// Must be included AFTER <windows.h>
#include <commdlg.h>
#include <process.h>
#include <shellapi.h>

#pragma warning(pop)

#include "core.h"
#include "error.h"

typedef struct Str16
{
    Char16 const *buf; // Pointer to string data (not a C string)
    Usize len;         // Lenght in chars of the string (not including terminators)
} Str16;

typedef CfArray(Char16) StrBuf16;

static inline Str16
str16FromCstr(Char16 *cstr)
{
    return (Str16){.buf = (cstr), .len = wcslen(cstr)};
}

static inline void
win32PrintLastError(void)
{
    DWORD error = GetLastError();
    LPSTR msg = NULL;
    FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
                       FORMAT_MESSAGE_IGNORE_INSERTS,
                   NULL, error, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&msg, 0, NULL);
    CF_DEBUG_BREAK();
    fprintf(stderr, "%s\n", msg);
    LocalFree(msg);
}

// UTF8<->UTF16 helpers

/// Encodes the given UTF8 string slice in UTF16.
/// The string is not null terminated, and the function returns the length of the written string in
/// UTF16 characters; in case of a NULL output buffer, this number is the minimum required buffer
/// size.
static inline Usize
win32Utf8To16(Str str, Char16 *out, Usize out_size)
{
    CF_ASSERT(out_size <= I32_MAX, "Invalid out size");

    I32 len =
        MultiByteToWideChar(CP_UTF8, MB_PRECOMPOSED, str.buf, (I32)str.len, out, (I32)out_size);

    if (len == 0)
    {
        win32PrintLastError();
        return USIZE_MAX;
    }

    CF_ASSERT(!out || (Usize)len <= out_size, "The given buffer is not large enough");

    return (Usize)len;
}

/// Encodes the given UTF8 string slice in UTF16.
/// The string is not null terminated, and the function returns the length of the written string in
/// bytes; in case of a NULL output buffer, this number is the minimum required buffer size.
static inline Usize
win32Utf16To8(Str16 str, Char8 *out, Usize out_size)
{
    CF_ASSERT(out_size <= I32_MAX, "Invalid out size");

    I32 len = WideCharToMultiByte(CP_UTF8, 0, str.buf, (I32)str.len, out, (I32)out_size, 0, false);

    if (len == 0)
    {
        win32PrintLastError();
        return USIZE_MAX;
    }

    CF_ASSERT(!out || (Usize)len <= out_size, "The given buffer is not large enough");

    return (Usize)len;
}
