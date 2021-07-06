#pragma once

/// Foundation core layer
/// This is the main API header, and is the only one that other API headers are allowed to include

#include <float.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
// TODO (Matteo): Get rid of it?
// At the moment it is required for printing assertion failures to stderr
#include <stdio.h>

// TODO (Matteo): This is growing fast and maybe should be trimmed

//------------------------------------------------------------------------------
//   MACROS
//------------------------------------------------------------------------------

/// Foundation utility macros that are at the base of the library and should be part
/// of the public API

// Note (Matteo): These are kept almost at top because are keywords
#define alignof _Alignof
#define alignas _Alignas

#define CF_UNUSED(var) (void)(var)

//-------------------//
// Context defines   //
//-------------------//

// NOTE (Matteo): compiler, platform, architecture

#if defined(__clang__)
#    define CF_COMPILER_CLANG 1
#elif defined(_MSC_VER)
#    define CF_COMPILER_MSVC 1
#elif defined(__GNUC__)
#    define CF_COMPILER_GCC 1
#    error "GCC not tested yet"
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

// NOTE (Matteo): Currently only little-endian systems are supported
#define CF_LITTLE_ENDIAN 1
#define CF_BIG_ENDIAN 0

#if !defined(CF_COMPILER_CLANG)
#    define CF_COMPILER_CLANG 0
#endif

#if !defined(CF_COMPILER_MSVC)
#    define CF_COMPILER_MSVC 0
#endif

#if !defined(CF_COMPILER_GCC)
#    define CF_COMPILER_GCC 0
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

#if CF_COMPILER_MSVC
#    define CF_MAX_ALIGN (sizeof(void *) * 2)
#else
#    define CF_MAX_ALIGN (alignof(max_align_t))
#endif

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

// NOTE (Matteo): Redundant but useful (maybe?)
#define CF_RELEASE !CF_DEBUG

// NOTE (Matteo): Memory protection is on by default, and can be disabled as a compilation flag
#if !defined(CF_MEMORY_PROTECTION)
#    define CF_MEMORY_PROTECTION CF_DEBUG
#endif

// NOTE (Matteo): Asserts in release builds are enabled by default, and can be disabled as a
// compilation flag
#if !defined(CF_RELEASE_ASSERTS)
#    define CF_RELEASE_ASSERTS 1
#endif

//-------------------//
//   Macro helpers   //
//-------------------//

#define CF__CONCAT(a, b) a##b
#define CF_CONCAT(a, b) CF__CONCAT(a, b)

#define CF__STRINGIFY(x) #x
#define CF_STRINGIFY(x) CF__STRINGIFY(x)

#define CF_MACRO_VAR(prefix) CF_CONCAT(prefix, CF_CONCAT(_, __LINE__))

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

//-----------------------//
// Basic utility macros  //
//-----------------------//

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

//------------------------------------------------------------------------------
//   TYPES
//------------------------------------------------------------------------------

/// Foundation types that can be used in API headers
/// Keep more specific types (e.g. arena allocators, threading primitives) in dedicated headers and
/// forward-declare them in API headers if required

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

#define TIME_INFINITE ((Time){.nanoseconds = I64_MIN})

#define TIME_NS(ns) \
    (CF_ASSERT(ns > I64_MIN, "Invalid nanoseconds count"), (Time){.nanoseconds = ns})
#define TIME_US(us) TIME_NS(1000 * us)
#define TIME_MS(ms) TIME_US(1000 * ms)

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

/// Immutable string slice/view. Not guaranteed to be null terminated.
/// Prefer it over C strings (safety, better API in progress)
typedef struct Str
{
    char const *buf; // Pointer to string data
    Usize len;       // Lenght in chars of the string (not including terminators)
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

//------------------------------------------------------------------------------