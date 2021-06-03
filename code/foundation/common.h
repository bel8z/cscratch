#ifndef FOUNDATION_COMMON_H

#include <float.h>
#include <stddef.h>
#include <stdint.h>

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
// Alignment

#define alignof _Alignof
#define alignas _Alignas

#if defined(_MSC_VER)
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

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

#define U8_MAX UINT8_MAX
#define U16_MAX UINT16_MAX
#define U32_MAX UINT32_MAX
#define U64_MAX UINT64_MAX

//------------------------------------------------------------------------------
// Fixed size signed integer types

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

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

typedef size_t usize;

#define USIZE_MAX SIZE_MAX

//------------------------------------------------------------------------------
// Signed integer type of the result of subtracting two pointers.

typedef ptrdiff_t isize;

#define ISIZE_MIN PTRDIFF_MIN
#define ISIZE_MAX PTRDIFF_MAX

//------------------------------------------------------------------------------
// Integer types capable of holding a pointer (for more comfortable arithmetics)

typedef intptr_t iptr;
typedef uintptr_t uptr;

#define UPTR_MAX UINTPTR_MAX
#define IPTR_MIN INTPTR_MIN
#define IPTR_MAX INTPTR_MAX

//------------------------------------------------------------------------------
// Fixed size IEEE floating point types

typedef float f32;
typedef double f64;

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
             i8 : I8_MIN,  \
             i16: I16_MIN, \
             i32: I32_MIN, \
             i64: I64_MIN, \
             f32: F32_MIN, \
             f64: F64_MIN)

#define T_MAX(Type)        \
    _Generic((Type)(0),    \
             u8 : U8_MAX,  \
             u16: U16_MAX, \
             u32: U32_MAX, \
             u64: U64_MAX, \
             i8 : I8_MAX,  \
             i16: I16_MAX, \
             i32: I32_MAX, \
             i64: I64_MAX, \
             f32: F32_MAX, \
             f64: F64_MAX)
// clang-format on

//------------------------------------------------------------------------------
// Forward declare commonly used foundation types so that they can appear in
// headers as pointers

// Macro for declaring a dynamic array (e.g. cfArray(i32) ints;) - see array.h
#define cfArray(Type) Type *

// Allocator abstract interface
typedef struct cfAllocator cfAllocator;

//------------------------------------------------------------------------------
// Vector maths types

typedef union Vec2 Vec2;
typedef union Vec3 Vec3;
typedef union Vec4 Vec4;

typedef f32 Mat4[4][4];

union Vec2
{
    struct
    {
        f32 x, y;
    };
    struct
    {
        f32 u, v;
    };
    f32 elem[2];
};

union Vec3
{
    struct
    {
        f32 x, y, z;
    };
    f32 elem[3];
};

union Vec4
{
    struct
    {
        f32 x, y, z, w;
    };
    f32 elem[4];
};

//------------------------------------------------------------------------------
// Color space types

// Represents a color in RGBA format as 4 floats in the [0,1] range
typedef union Rgba Rgba;

// Represents a color in HSV format, plus alpha channel, as 4 floats in the [0,1] range
typedef union Hsva Hsva;

// Packed RBGA representation
typedef u32 Rgba32;

union Rgba
{
    struct
    {
        f32 r, g, b, a;
    };

    f32 channel[4];
};

union Hsva
{
    struct
    {
        f32 h, s, v, a;
    };

    f32 elem[4];
};

//------------------------------------------------------------------------------
// Time interval, useful for performance tracking

typedef struct Time
{
    i64 nanoseconds;
} Time;

#define TIME_INFINITE ((Time){.nanoseconds = I64_MIN})
#define TIME_IS_INFINITE(time) (time.nanoseconds == I64_MIN)

#define TIME_MS(ms) TIME_US(1000 * ms)
#define TIME_US(us) TIME_NS(1000 * us)
#define TIME_NS(ns) ((Time){.nanoseconds = ns})

#define timeAdd(a, b) ((Time){.nanoseconds = a.nanoseconds + b.nanoseconds})
#define timeSub(a, b) ((Time){.nanoseconds = a.nanoseconds - b.nanoseconds})

//------------------------------------------------------------------------------
// Assertion macros

#if CF_RELEASE_ASSERTS && defined(NDEBUG)
#define CF__RESTORE_NDEBUG 1
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

#define CF_KB(x) ((u64)1024 * (x))
#define CF_MB(x) ((u64)1024 * CF_KB(x))
#define CF_GB(x) ((u64)1024 * CF_MB(x))

//------------------------------------------------------------------------------

#define FOUNDATION_COMMON_H
#endif
