#pragma once

/// Foundation types that can be used in API headers
/// Keep more specific types (e.g. arena allocators, threading primitives) in dedicated headers and
/// forward-declare them in API headers if required

#include <float.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

//-------------//
//   Boolean   //
//-------------//

#ifdef bool
#    undef bool
typedef _Bool bool;
#endif

//----------------------------------//
//   Fixed size unsigned integers   //
//----------------------------------//

typedef uint8_t U8;
typedef uint16_t U16;
typedef uint32_t U32;
typedef uint64_t U64;

#define U8_MAX UINT8_MAX
#define U16_MAX UINT16_MAX
#define U32_MAX UINT32_MAX
#define U64_MAX UINT64_MAX

//--------------------------------//
//   Fixed size signed integers   //
//--------------------------------//

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

// Unsigned integer type of the result of sizeof, alignof and offsetof.

typedef size_t Usize;

#define USIZE_MAX SIZE_MAX

// Signed integer type of the result of subtracting two pointers.

typedef ptrdiff_t Isize;

#define ISIZE_MIN PTRDIFF_MIN
#define ISIZE_MAX PTRDIFF_MAX

// Integer types capable of holding a pointer (for more comfortable arithmetics)

typedef intptr_t Iptr;
typedef uintptr_t Uptr;

#define UPTR_MAX UINTPTR_MAX
#define IPTR_MIN INTPTR_MIN
#define IPTR_MAX INTPTR_MAX

// Fixed size IEEE floating point types

typedef float F32;
typedef double F64;

#define F32_MIN FLT_MIN
#define F32_MAX FLT_MAX
#define F32_EPS FLT_EPSILON

#define F64_MIN DBL_MIN
#define F64_MAX DBL_MAX
#define F64_EPS DBL_EPSILON

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

//----------------------------------------------//
//   Time interval (useful for perf counting)   //
//----------------------------------------------//

typedef struct Time
{
    I64 nanoseconds;
} Time;

//---------------//
//   Allocator   //
//---------------//

/// Definition of the main allocation function
#define CF_ALLOCATOR_FUNC(name) \
    void *name(void *state, void *memory, Usize old_size, Usize new_size, Usize align)

/// Generic allocator interface
/// The memory provided by this interface should already be cleared to 0
typedef struct cfAllocator
{
    void *state;
    CF_ALLOCATOR_FUNC((*func));
} cfAllocator;

//-------------------//
//   Dynamic array   //
//-------------------//

/// Macro to define a typed dynamic array (variable or typedef)
/// Functionality is implemented in array.h
#define cfArray(Type)                                                                              \
    struct                                                                                         \
    {                                                                                              \
        /* Allocator used for growing the array dynamically */                                     \
        cfAllocator alloc;                                                                         \
        /* Actual array storage */                                                                 \
        Type *buf;                                                                                 \
        /* Size of the array (number of stored items) */                                           \
        Usize len;                                                                                 \
        /* Capacity of the array (number of elements that can be stored before the array grows) */ \
        Usize cap;                                                                                 \
    }

//-------------//
//   Strings   //
//-------------//

/// Dynamic string buffer
// TODO (Matteo): Make a separate type for better API separation?
typedef cfArray(char) StrBuffer;

/// Better(ish) string representation (not necessarily null terminated)
typedef struct Str
{
    char *buf; // Pointer to string data
    Usize len; // Lenght in chars of the string (not including terminators)
} Str;

//-------------//
//   Vectors   //
//-------------//

/// Affinity matrix (floating point)
typedef F32 Mat4[4][4];

// TODO (Matteo): Maybe rename VecN to FVecN?

/// 2D vector of F32s
typedef union Vec2
{
    struct
    {
        F32 x, y;
    };
    struct
    {
        F32 u, v;
    };
    struct
    {
        F32 width, height;
    };
    F32 elem[2];
} Vec2;

/// 3D vector of F32s
typedef union Vec3
{
    struct
    {
        F32 x, y, z;
    };
    struct
    {
        Vec2 xy;
        F32 _;
    };
    F32 elem[3];
} Vec3;

/// 4D vector of F32s (quaternion)
typedef union Vec4
{
    struct
    {
        F32 x, y, z, w;
    };
    struct
    {
        Vec3 xyz;
        F32 _;
    };
    F32 elem[4];
} Vec4;

/// 2D vector of I32s
typedef union IVec2
{
    struct
    {
        I32 x, y;
    };
    struct
    {
        I32 u, v;
    };
    struct
    {
        F32 width, height;
    };
    I32 elem[2];
} IVec2;

/// 3D vector of I32s
typedef union IVec3
{
    struct
    {
        I32 x, y, z;
    };
    struct
    {
        Vec2 xy;
        I32 _;
    };
    I32 elem[3];
} IVec3;

/// 4D vector of I32s (quaternion)
typedef union IVec4
{
    struct
    {
        I32 x, y, z, w;
    };
    struct
    {
        IVec3 xyz;
        I32 _;
    };

    I32 elem[4];
} IVec4;

//------------------//
//   Color spaces   //
//------------------//

/// Packed RBGA representation
typedef U32 Rgba32;

/// Represents a color in RGBA format as 4 floats in the [0,1] range
typedef union Rgba
{
    struct
    {
        F32 r, g, b, a;
    };
    F32 channel[4];
} Rgba;

/// Represents a color in HSV format, plus alpha channel, as 4 floats in the [0,1] range
typedef union Hsva
{
    struct
    {
        F32 h, s, v, a;
    };
    F32 elem[4];
} Hsva;

//----------------//
//   Rectangles   //
//----------------//

typedef union FRect
{
    struct
    {
        Vec2 p0, p1;
    };
    struct
    {
        F32 x0, y0, x1, y1;
    };
} FRect;

typedef union IRect
{
    struct
    {
        IVec2 p0, p1;
    };
    struct
    {
        I32 x0, y0, x1, y1;
    };
} IRect;
