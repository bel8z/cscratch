#ifndef FOUNDATION_COMMON_H

#include <float.h>
#include <stddef.h>
#include <stdint.h>

//------------------------------------------------------------------------------
// Customization flags

// NOTE (Matteo): Memory protection is on by default, and can be disabled as a compilation flag
#if !defined(CF_MEMORY_PROTECTION)
#define CF_MEMORY_PROTECTION 1
#endif

// NOTE (Matteo): Asserts in release builds are enabled by default, and can be disabled as a
// compilation flag
#if !defined(CF_RELEASE_ASSERTS)
#define CF_RELEASE_ASSERTS 1
#endif

//------------------------------------------------------------------------------
// Context defines

// clang-format off
#if defined(__clang__)
#    define CF_COMPILER_CLANG 1
#elif defined(_MSC_VER)
#    define CF_COMPILER_MSVC 1
#elif defined(__GNUC__)
#    define CF_COMPILER_GCC 1
#else
#    error "Compiler not detected"
#endif

#if defined(_WIN32)
#    define CF_OS_WIN32 1
#elif defined(__gnu_linux__)
#    define CF_OS_LINUX 1
#elif defined(__APPLE__) && defined(__MACH__)
#    define CF_OS_MAC 1
#else
#    error "OS not detected"
#endif

#if defined(CF_COMPILER_MSVC)
#    if defined(_M_X64)
#        define CF_ARCH_X64 1
#    elif defined(_M_IX86)
#        define CF_ARCH_X86 1
#    else
#        error "Architecture not detected"
#    endif
#else
#    if defined(__i386)
#        define CF_ARCH_X86 1
#    elif defined(__amd64)
#        define CF_ARCH_X64 1
#    else
#        error "Architecture not detected"
#    endif
#endif

#if !defined(CF_COMPILER_CLANG)
#    define CF_COMPILER_CLANG 0
#endif

#if !defined(CF_COMPILER_GCC)
#    define CF_COMPILER_GCC 0
#endif

#if !defined(CF_COMPILER_MSVC)
#    define CF_COMPILER_MSVC 0
#endif

#if !defined(CF_OS_WIN32)
#    define CF_OS_WIN32 0
#endif

#if !defined(CF_OS_LINUX)
#    define CF_OS_LINUX 0
#endif

#if !defined(CF_OS_MAC)
#    define CF_OS_MAC 0
#endif

#if !defined(CF_ARCH_X64)
#    define CF_ARCH_X64 0
#endif

#if !defined(CF_ARCH_X86)
#    define CF_ARCH_X86 0
#endif
// clang-format on

//------------------------------------------------------------------------------
// Alignment

#define alignof _Alignof
#define alignas _Alignas

#if CF_COMPILER_MSVC
#define CF_MAX_ALIGN (sizeof(void *) * 2)
#else
#define CF_MAX_ALIGN (alignof(max_align_t))
#endif

//------------------------------------------------------------------------------
// Boolean type

#include <stdbool.h>

#ifdef bool
#undef bool
typedef _Bool bool;
#endif

//------------------------------------------------------------------------------
// Fixed size unsigned integer types

typedef uint8_t U8;
typedef uint16_t U16;
typedef uint32_t U32;
typedef uint64_t U64;

#define U8_MAX UINT8_MAX
#define U16_MAX UINT16_MAX
#define U32_MAX UINT32_MAX
#define U64_MAX UINT64_MAX

//------------------------------------------------------------------------------
// Fixed size signed integer types

typedef int8_t I8;
typedef int16_t I16;
typedef int32_t I32;
typedef int64_t I64;

#define I8_MIN INT8_MIN
#define I8_MAX INT8_MAX

#define I16_MIN INT16_MIN
#define I16_MAX INT16_MAX

#define I32_MIN INT32_MIN
#define I32_MAX INT32_MAX

#define I64_MIN INT64_MIN
#define I64_MAX INT64_MAX

//------------------------------------------------------------------------------
// Unsigned integer type of the result of sizeof, alignof and offsetof.

typedef size_t Usize;

#define USIZE_MAX SIZE_MAX

//------------------------------------------------------------------------------
// Signed integer type of the result of subtracting two pointers.

typedef ptrdiff_t Isize;

#define ISIZE_MIN PTRDIFF_MIN
#define ISIZE_MAX PTRDIFF_MAX

//------------------------------------------------------------------------------
// Integer types capable of holding a pointer (for more comfortable arithmetics)

typedef intptr_t Iptr;
typedef uintptr_t Uptr;

#define UPTR_MAX UINTPTR_MAX
#define IPTR_MIN INTPTR_MIN
#define IPTR_MAX INTPTR_MAX

//------------------------------------------------------------------------------
// Fixed size IEEE floating point types

typedef float F32;
typedef double F64;

#define F32_MIN FLT_MIN
#define F32_MAX FLT_MAX
#define F32_EPS FLT_EPSILON

#define F64_MIN DBL_MIN
#define F64_MAX DBL_MAX
#define F64_EPS DBL_EPSILON

//------------------------------------------------------------------------------
// Macros to retrieve min/max values for basic types

// TODO (Matteo): Find a better name

// clang-format off
#define T_MIN(Type)        \
    _Generic((Type)(0),    \
             I8 : I8_MIN,  \
             I16: I16_MIN, \
             I32: I32_MIN, \
             I64: I64_MIN, \
             F32: F32_MIN, \
             F64: F64_MIN)

#define T_MAX(Type)        \
    _Generic((Type)(0),    \
             U8 : U8_MAX,  \
             U16: U16_MAX, \
             U32: U32_MAX, \
             U64: U64_MAX, \
             I8 : I8_MAX,  \
             I16: I16_MAX, \
             I32: I32_MAX, \
             I64: I64_MAX, \
             F32: F32_MAX, \
             F64: F64_MAX)
// clang-format on

//------------------------------------------------------------------------------
// Forward declare commonly used foundation types so that they can appear in
// headers as pointers

// Macro for declaring a dynamic array (e.g. cfArray(I32) ints;) - see array.h
#define cfArray(Type) Type *

// Allocator abstract interface
typedef struct cfAllocator cfAllocator;

//------------------------------------------------------------------------------
// Vector maths types

typedef union Vec2 Vec2;
typedef union Vec3 Vec3;
typedef union Vec4 Vec4;

typedef F32 Mat4[4][4];

union Vec2
{
    struct
    {
        F32 x, y;
    };
    struct
    {
        F32 u, v;
    };
    F32 elem[2];
};

union Vec3
{
    struct
    {
        F32 x, y, z;
    };
    F32 elem[3];
};

union Vec4
{
    struct
    {
        F32 x, y, z, w;
    };
    F32 elem[4];
};

//------------------------------------------------------------------------------
// Color space types

// Represents a color in RGBA format as 4 floats in the [0,1] range
typedef union Rgba Rgba;

// Represents a color in HSV format, plus alpha channel, as 4 floats in the [0,1] range
typedef union Hsva Hsva;

// Packed RBGA representation
typedef U32 Rgba32;

union Rgba
{
    struct
    {
        F32 r, g, b, a;
    };

    F32 channel[4];
};

union Hsva
{
    struct
    {
        F32 h, s, v, a;
    };

    F32 elem[4];
};

//------------------------------------------------------------------------------
// Time interval, useful for performance tracking

typedef struct Time
{
    I64 nanoseconds;
} Time;

#define TIME_INFINITE ((Time){.nanoseconds = I64_MIN})
#define TIME_IS_INFINITE(time) (time.nanoseconds == I64_MIN)

#define TIME_NS(ns) \
    (CF_ASSERT(ns > I64_MIN, "Invalid nanoseconds count"), (Time){.nanoseconds = ns})
#define TIME_US(us) TIME_NS(1000 * us)
#define TIME_MS(ms) TIME_US(1000 * ms)

#define timeAdd(a, b) ((Time){.nanoseconds = a.nanoseconds + b.nanoseconds})
#define timeSub(a, b) ((Time){.nanoseconds = a.nanoseconds - b.nanoseconds})

//------------------------------------------------------------------------------
// Assertion macros

#if CF_RELEASE_ASSERTS && defined(NDEBUG)
#define CF__RESTORE_NDEBUG
#undef NDEBUG
#endif

#include <assert.h>

#define CF_STATIC_ASSERT(expr, msg) _Static_assert(expr, msg)

#if defined(NDEBUG)
#define CF_ASSERT(expr, msg) (CF_UNUSED(expr), CF_UNUSED(msg))
#else
#define CF_ASSERT(expr, msg) (assert((expr) && (msg)))
#endif

#define CF_ASSERT_NOT_NULL(ptr) CF_ASSERT(ptr, #ptr " is null")

#define CF_NOT_IMPLEMENTED() CF_ASSERT(false, "Not implemented")

#define CF_INVALID_CODE_PATH() CF_ASSERT(false, "Invalid code path")

#if defined(CF__RESTORE_NDEBUG)
#define NDEBUG 1
#undef CF__RESTORE_NDEBUG
#endif

//------------------------------------------------------------------------------
// Misc

#define CF_UNUSED(var) (void)(var)

#define CF_ARRAY_SIZE(a) sizeof(a) / sizeof(a[0])

#define CF_KB(x) ((U64)1024 * (x))
#define CF_MB(x) ((U64)1024 * CF_KB(x))
#define CF_GB(x) ((U64)1024 * CF_MB(x))

#define CF__CONCAT(a, b) a##b
#define CF_CONCAT(a, b) CF__CONCAT(a, b)

#define CF__STRINGIFY(x) #x
#define CF_STRINGIFY(x) CF__STRINGIFY(x)

//------------------------------------------------------------------------------

#define FOUNDATION_COMMON_H
#endif
