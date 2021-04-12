#ifndef CF_COMMON_H

#include <float.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

//------------------------------------------------------------------------------
// Boolean type

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
// Assertion macros

// Ensure assertion in release mode
#define RELEASE_ASSERTS 1

#if defined(RELEASE_ASSERTS) && defined(NDEBUG)
#define RESTORE_NDEBUG
#undef NDEBUG
#endif

#include <assert.h>

#define CF_STATIC_ASSERT(expr, msg) _Static_assert(expr, msg)
#define CF_ASSERT(expr, msg) ((void)(expr), assert((expr) && (msg)))
#define CF_ASSERT_NOT_NULL(array) CF_ASSERT(array, "Array is null")

#if defined(RESTORE_NDEBUG)
#define NDEBUG 1
#undef RESTORE_NDEBUG
#endif
//------------------------------------------------------------------------------

#define CF_COMMON_H
#endif