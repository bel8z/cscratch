#pragma once

// TODO (Matteo): Get rid of it?
// At the moment it is required for printing assertion failures to stderr
#include <stdio.h>

//-------------------------------//
//   Assertions / Debug macros   //
//-------------------------------//

#if CF_COMPILER_CLANG
#    define CF__CRASH() __builtin_trap()
#else
#    define CF__CRASH() *((int *)0) = 0
#endif

#define CF__ERROR_PRINT(msg) \
    fprintf(stderr, "Unexpected error - %s\nFile: %s\nLine: %d\n", msg, CF_FILE, CF_LINE)

#define CF__ASSERT_PRINT(expr, msg)                                                             \
    fprintf(stderr, "Assertion failed - %s: %s\nFile: %s\nLine: %d\n", msg, CF_STRINGIFY(expr), \
            CF_FILE, CF_LINE)

/// CF_CRASH: Guaranteed program crash + error reporting
#define CF_CRASH(msg) ((CF__ERROR_PRINT(msg), CF__CRASH(), 0))

/// CF_DEBUG_BREAK: Break execution in debug mode
#if CF_DEBUG
#    if CF_COMPILER_MSVC
#        define CF_DEBUG_BREAK() __debugbreak()
#    elif CF_COMPILER_CLANG
#        define CF_DEBUG_BREAK() __builtin_debugtrap()
#    elif CF_COMPILER_GCC
#        define CF_DEBUG_BREAK() __builtin_trap()
#    else
#        define CF_DEBUG_BREAK() CF__CRASH()
#    endif
#else
#    define CF_DEBUG_BREAK()
#endif

#if CF_DEBUG
#    define CF__ASSERT_FAIL(expr, msg) ((CF__ASSERT_PRINT(expr, msg), CF_DEBUG_BREAK(), 0))
#else
#    define CF__ASSERT_FAIL(expr, msg) ((CF__ASSERT_PRINT(expr, msg), CF__CRASH(), 0))
#endif

/// CF_ASSERT: Assertion macro, by default enabled in release builds (use CF_RELEASE_ASSERTS to
/// disable)
#if CF_DEBUG || CF_RELEASE_ASSERTS
#    define CF_ASSERT(expr, msg) (!(expr) ? CF__ASSERT_FAIL(expr, msg) : 1)
#else
#    define CF_ASSERT(expr, msg) CF_UNUSED(expr)
#endif

/// CF_ASSERT_FAIL: Assertion failure + error report
#if CF_DEBUG
#    define CF_ASSERT_FAIL(msg) ((CF__ERROR_PRINT(msg), CF_DEBUG_BREAK(), 0))
#elif CF_RELEASE_ASSERTS
#    define CF_ASSERT_FAIL(msg) CF_CRASH(msg)
#else
#    define CF_ASSERT_FAIL(msg)
#endif

#define CF_ASSERT_NOT_NULL(ptr) CF_ASSERT(ptr, #ptr " is null")

#define CF_NOT_IMPLEMENTED() CF_ASSERT_FAIL("Not implemented")

#define CF_INVALID_CODE_PATH() CF_ASSERT_FAIL("Invalid code path")

/// CF_DEBUG_ASSERT: Assertion macro enabled in debug builds only
#if CF_DEBUG
#    define CF_DEBUG_ASSERT(expr, msg) CF_ASSERT(expr, msg)
#else
#    define CF_DEBUG_ASSERT(expr, msg) CF_UNUSED(expr)
#endif
