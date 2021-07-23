// Attempt to include windows headers in a clean way

#pragma once

#pragma warning(push)
#pragma warning(disable : 5105)

#define NOMINMAX 1
#define VC_EXTRALEAN 1
#define WIN32_LEAN_AND_MEAN 1
#include <Windows.h>
// Must be included AFTER <windows.h>
#include <commdlg.h>
#include <process.h>
#include <shellapi.h>

#pragma warning(pop)

#include "core.h"

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

/// Encodes the given UTF8 string slice in UTF16 and null terminates it. The function returns the
/// number of bytes written (including the null terminator); in case of a NULL output buffer, this
/// number is the minimum required buffer size.
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

    // NOTE (Matteo): Since the input string length is given, the output string is not
    // null-terminated and as such the terminator is not included in the write count
    if (out)
    {
        CF_ASSERT((Usize)len < out_size, "The given buffer is not large enough");
        out[len] = 0;
    }

    return (Usize)(len + 1);
}

/// Encodes the given UTF8 C string in UTF16 and null terminates it. The function returns the
/// number of bytes written (including the null terminator); in case of a NULL output buffer, this
/// number is the minimum required buffer size.
static inline Usize
win32Utf8To16C(Cstr cstr, Char16 *out, Usize out_size)
{
    CF_ASSERT(out_size <= I32_MAX, "Invalid out size");

    I32 result = MultiByteToWideChar(CP_UTF8, MB_PRECOMPOSED, cstr, -1, out, (I32)out_size);
    // NOTE (Matteo): Since the input string is null-terminated, the terminator is included in the
    // size count
    if (result) return (Usize)result;

    win32PrintLastError();
    return USIZE_MAX;
}

/// Encodes the given UTF16 C string in UTF8 and null terminates it. The function returns the
/// number of bytes written (including the null terminator); in case of a NULL output buffer, this
/// number is the minimum required buffer size.
static inline Usize
win32Utf16To8C(Char16 const *str, Char8 *out, Usize out_size)
{
    CF_ASSERT(out_size <= I32_MAX, "Invalid out size");

    I32 result = WideCharToMultiByte(CP_UTF8, 0, str, -1, out, (I32)out_size, 0, false);
    // NOTE (Matteo): Since the input string is null-terminated, the terminator is included in the
    // size count
    if (result) return (Usize)result;

    win32PrintLastError();
    return USIZE_MAX;
}
