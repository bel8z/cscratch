#pragma once

// This is the base layer of the library, and should be the only header included by other header
// files
// TODO (Matteo): This is growing fast and maybe should be trimmed

#include "core/context.h"
#include "core/types.h"

#include <stdio.h>
#include <string.h>

#define CF_UNUSED(var) (void)(var)

//-------------------------//
// Configuration defines   //
//-------------------------//

#if !defined(CF_DEBUG)
#    if CF_COMPILER_MSVC
#        if defined(NDEBUG) && !defined(_DEBUG)
#            define CF_DEBUG 0
#        else
#            define CF_DEBUG 1
#        endif
#    elif defined NDEBUG
#        define CF_DEBUG 0
#    else
#        define CF_DEBUG 1
#    endif
#endif

// NOTE (Matteo): Memory protection is on by default, and can be disabled as a compilation flag
#if !defined(CF_MEMORY_PROTECTION)
#    define CF_MEMORY_PROTECTION CF_DEBUG
#endif

// NOTE (Matteo): Asserts in release builds are enabled by default, and can be disabled as a
// compilation flag
#if !defined(CF_RELEASE_ASSERTS)
#    define CF_RELEASE_ASSERTS 1
#endif

//-------------------------------//
//   Assertions / Debug macros   //
//-------------------------------//

/// Compile time assertion
#define CF_STATIC_ASSERT(expr, msg) _Static_assert(expr, msg)

#if CF_COMPILER_CLANG
#    define CF_CRASH() __builtin_trap()
#else
#    define CF_CRASH() *((int *)0) = 0
#endif

/// Break execution in debug mode

#if CF_DEBUG
#    if CF_COMPILER_MSVC
#        define CF_DEBUG_BREAK() __debugbreak()
#    elif CF_COMPILER_CLANG
#        define CF_DEBUG_BREAK() __builtin_debugtrap()
#    elif CF_COMPILER_GCC
#        define CF_DEBUG_BREAK() __builtin_trap()
#    else
#        define CF_DEBUG_BREAK() CF_CRASH()
#    endif
#else
#    define CF_DEBUG_BREAK()
#endif

/// CF_ASSERT
/// Assertion macro, by default enabled in release builds (use CF_RELEASE_ASSERTS to disable)

#define CF__ASSERT_PRINT(msg) \
    fprintf(stderr, "Assertion failed: %s\nFile: %s\nLine: %d\n", msg, __FILE__, __LINE__)

#if CF_DEBUG
#    define CF_ASSERT(expr, msg) (!(expr) ? (CF__ASSERT_PRINT(msg), CF_DEBUG_BREAK(), 0) : 1)
#elif CF_RELEASE_ASSERTS
#    define CF_ASSERT(expr, msg) (!(expr) ? (CF__ASSERT_PRINT(msg), CF_CRASH(), 0) : 1)
#else
#    define CF_ASSERT(expr, msg) CF_UNUSED(expr)
#endif

#define CF_ASSERT_NOT_NULL(ptr) CF_ASSERT(ptr, #ptr " is null")

#define CF_NOT_IMPLEMENTED() CF_ASSERT(false, "Not implemented")

#define CF_INVALID_CODE_PATH() CF_ASSERT(false, "Invalid code path")

/// Assertion macro enabled in debug builds only
#if CF_DEBUG
#    define CF_DEBUG_ASSERT(expr, msg) CF_ASSERT(expr)
#else
#    define CF_DEBUG_ASSERT(expr, msg) CF_UNUSED(expr)
#endif

//-----------------//
//   Macro magic   //
//-----------------//

#define CF__CONCAT(a, b) a##b
#define CF_CONCAT(a, b) CF__CONCAT(a, b)

#define CF__STRINGIFY(x) #x
#define CF_STRINGIFY(x) CF__STRINGIFY(x)

#define CF_MACRO_VAR(prefix) CF_CONCAT(prefix, CF_CONCAT(_, __LINE__))

//-------------------//
// Basic utilities   //
//-------------------//

#define CF_ARRAY_SIZE(a) sizeof(a) / sizeof(a[0])

#define CF_KB(x) ((U64)1024 * (x))
#define CF_MB(x) ((U64)1024 * CF_KB(x))
#define CF_GB(x) ((U64)1024 * CF_MB(x))

#define cfClamp(val, min_val, max_val) \
    ((val) < (min_val) ? (min_val) : (val) > (max_val) ? (max_val) : (val))

#define cfMin(l, r) ((l) < (r) ? (l) : (r))
#define cfMax(l, r) ((l) > (r) ? (l) : (r))

/// Actual integer modulo operation (% is a remainder operation and as such can produce negative
/// values)
#define cfMod(val, len) ((val % len + len) % len)

/// Increment value wrapping on the given length
#define cfWrapInc(val, len) (val == (len - 1) ? 0 : (val + 1)) // ((val + 1) % len)

/// Decrement value wrapping on the given length
#define cfWrapDec(val, len) (val == 0 ? (len - 1) : (val - 1)) // ((val + len - 1) % len)

//--------------------//
//   Time utilities   //
//--------------------//

// NOTE (Matteo): those are mainly for performance counting

#define TIME_INFINITE ((Time){.nanoseconds = I64_MIN})
#define TIME_IS_INFINITE(time) (time.nanoseconds == I64_MIN)

#define TIME_NS(ns) \
    (CF_ASSERT(ns > I64_MIN, "Invalid nanoseconds count"), (Time){.nanoseconds = ns})
#define TIME_US(us) TIME_NS(1000 * us)
#define TIME_MS(ms) TIME_US(1000 * ms)

#define timeAdd(a, b) ((Time){.nanoseconds = a.nanoseconds + b.nanoseconds})
#define timeSub(a, b) ((Time){.nanoseconds = a.nanoseconds - b.nanoseconds})

//----------------------//
//   Memory utilities   //
//----------------------//

#define cfAlloc(a, size) cfAllocAlign(a, size, CF_MAX_ALIGN)
#define cfAllocAlign(a, size, align) (a).func((a).state, NULL, 0, (size), (align))

#define cfReallocAlign(a, mem, old_size, new_size, align) \
    (a).func((a).state, (mem), (old_size), (new_size), (align))
#define cfRealloc(a, mem, old_size, new_size) \
    cfReallocAlign(a, mem, old_size, new_size, CF_MAX_ALIGN)

#define cfFree(a, mem, size) (a).func((a).state, (void *)(mem), (size), 0, 0)

#define cfMemClear(mem, count) memset(mem, 0, count)        // NOLINT
#define cfMemCopy(from, to, count) memmove(to, from, count) // NOLINT
#define cfMemCopySafe(from, from_size, to, to_size) memmove_s(to, to_size, from, from_size)

static inline void
cfMemWrite(U8 *mem, U8 value, Usize count)
{
    memset(mem, value, count); // NOLINT
}

//------------------------------------------------------------------------------
