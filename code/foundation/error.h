#pragma once

// TODO (Matteo): Get rid of it?
// At the moment it is required for printing assertion failures to stderr
#include <stdio.h>

//-------------------------------//
//   Assertions / Debug macros   //
//-------------------------------//

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

#define CF__ASSERT_PRINT(expr, msg)                                                             \
    fprintf(stderr, "Assertion failed - %s: %s\nFile: %s\nLine: %d\n", msg, CF_STRINGIFY(expr), \
            CF_FILE, CF_LINE)

#if CF_DEBUG
#    define CF_FAIL(msg) ((CF__ASSERT_PRINT(expr, msg), CF_DEBUG_BREAK(), 0))
#else
#    define CF_FAIL(msg) ((CF__ASSERT_PRINT(expr, msg), CF_CRASH(), 0))
#endif

#if CF_DEBUG || CF_RELEASE_ASSERTS
#    define CF_ASSERT(expr, msg) (!(expr) ? CF_FAIL(msg) : 1)
#else
#    define CF_ASSERT(expr, msg) CF_UNUSED(expr)
#endif

#define CF_ASSERT_NOT_NULL(ptr) CF_ASSERT(ptr, #ptr " is null")

#define CF_NOT_IMPLEMENTED() CF_ASSERT(false, "Not implemented")

#define CF_INVALID_CODE_PATH() CF_ASSERT(false, "Invalid code path")

/// Assertion macro enabled in debug builds only
#if CF_DEBUG
#    define CF_DEBUG_ASSERT(expr, msg) CF_ASSERT(expr, msg)
#else
#    define CF_DEBUG_ASSERT(expr, msg) CF_UNUSED(expr)
#endif
