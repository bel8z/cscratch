#pragma once

/// Foundation core layer
/// This is the main API header, and is the only one that other API headers are allowed to include

#include <float.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// TODO (Matteo): This is growing fast and maybe should be trimmed

//------------------------------------------------------------------------------
//   MACROS
//------------------------------------------------------------------------------

/// Foundation utility macros that are at the base of the library and should be part
/// of the public API

// Note (Matteo): These are kept almost at top because are keywords
#if !defined(__cplusplus)
#    define alignof _Alignof
#    define alignas _Alignas
#    define CF_STATIC_ASSERT _Static_assert
#else
#    define CF_STATIC_ASSERT static_assert
#endif

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

#if CF_ARCH_X86
#    define CF_PTR_SIZE 4
#    define CF_CACHELINE_SIZE 64
#elif CF_ARCH_X64
#    define CF_PTR_SIZE 8
#    define CF_CACHELINE_SIZE 64
#else
#    error "Architecture not detected"
#endif

#if CF_COMPILER_MSVC
#    if CF_ARCH_X64
#        define CF_MAX_ALIGN 16
#    else
#        define CF_MAX_ALIGN 8
#    endif
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

#if !defined(CF_SANITIZE)
#    define CF_SANITIZE 0
#endif

#if defined(__cplusplus)
#    define CF_EXTERN_C extern "C"
#else
#    define CF_EXTERN_C extern
#endif

#if CF_OS_WIN32
#    define CF_DLL_EXPORT __declspec(dllexport)
#else
#    define CF_DLL_EXPORT
#endif

// Exported function
#define CF_API CF_EXTERN_C CF_DLL_EXPORT

// Function with both inline and exported out-of-line declaration
#define CF_INLINE_API CF_EXTERN_C inline CF_DLL_EXPORT

//-------------------//
//   Compiler diagnostics   //
//-------------------//

// NOTE (Matteo): Clang macros apply also to GCC

#if CF_COMPILER_CLANG
#    define CF_DIAGNOSTIC_PUSH() _Pragma("clang diagnostic push")
#    define CF_DIAGNOSTIC_POP() _Pragma("clang diagnostic pop")
#    define CF_DIAGNOSTIC_IGNORE_CLANG(x) _Pragma(CF_STRINGIFY(clang diagnostic ignored x))
#    define CF_DIAGNOSTIC_IGNORE_MSVC(x)
#    define CF_DIAGNOSTIC_RESTORE_CLANG(x) _Pragma(CF_STRINGIFY(clang diagnostic warning x))
#    define CF_DIAGNOSTIC_RESTORE_MSVC(x)
#elif CF_COMPILER_MSVC
#    define CF_DIAGNOSTIC_PUSH() _Pragma("warning(push)")
#    define CF_DIAGNOSTIC_POP() _Pragma("warning(pop)")
#    define CF_DIAGNOSTIC_IGNORE_CLANG(x)
#    define CF_DIAGNOSTIC_IGNORE_MSVC(x) _Pragma(CF_STRINGIFY(warning(disable : x)))
#    define CF_DIAGNOSTIC_RESTORE_CLANG(x)
#    define CF_DIAGNOSTIC_RESTORE_MSVC(x) _Pragma(CF_STRINGIFY(warning(default : x)))
#elif CF_COMPILER_GCC
#    define CF_DIAGNOSTIC_PUSH() _Pragma("gcc diagnostic push")
#    define CF_DIAGNOSTIC_POP() _Pragma("gcc diagnostic pop")
#    define CF_DIAGNOSTIC_IGNORE_CLANG(x) _Pragma(CF_STRINGIFY(gcc diagnostic ignored x))
#    define CF_DIAGNOSTIC_IGNORE_MSVC(x)
#    define CF_DIAGNOSTIC_RESTORE_CLANG(x) _Pragma(CF_STRINGIFY(gcc diagnostic warning x))
#    define CF_DIAGNOSTIC_RESTORE_MSVC(x)
#else
#    define CF_DIAGNOSTIC_PUSH()
#    define CF_DIAGNOSTIC_POP()
#    define CF_DIAGNOSTIC_IGNORE_CLANG(x)
#    define CF_DIAGNOSTIC_IGNORE_MSVC(x)
#    define CF_DIAGNOSTIC_RESTORE_CLANG(x)
#    define CF_DIAGNOSTIC_RESTORE_MSVC(x)
#endif

//-------------------//
//   Macro helpers   //
//-------------------//

#define CF_FILE __FILE__
#define CF_LINE __LINE__

#define CF__CONCAT(a, b) a##b
#define CF_CONCAT(a, b) CF__CONCAT(a, b)

#define CF__STRINGIFY(x) #x
#define CF_STRINGIFY(x) CF__STRINGIFY(x)

#define CF_MACRO_VAR(prefix) CF_CONCAT(prefix, CF_CONCAT(_, CF_LINE))

#if CF_COMPILER_CLANG
#    define CF_PRINTF_LIKE(fmt_argno) \
        __attribute__((__format__(__printf__, fmt_argno + 1, fmt_argno + 2)))
#    define CF_VPRINTF_LIKE(fmt_argno) __attribute__((__format__(__printf__, fmt_argno + 1, 0)))
#else
#    define CF_PRINTF_LIKE(fmt_argno)
#    define CF_VPRINTF_LIKE(fmt_argno)
#endif

//-----------------------//
// Basic utility macros  //
//-----------------------//

#define CF_ARRAY_SIZE(a) (sizeof(a) / sizeof(a[0]))

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

/// Check at compile time that the two arguments have the same type
#define CF_SAME_TYPE(l, r) (0 && ((l) = (r), 0))

#define CF_PAD(size) U8 CF_MACRO_VAR(_pad)[size]
#define CF_CACHELINE_PAD CF_PAD(CF_CACHELINE_SIZE)

/// Check if the given integer is a power of 2
#define cfIsPowerOf2(value) (!((value) & ((value)-1)))

//------------------------------------------------------------------------------
//   TYPES
//------------------------------------------------------------------------------

#if defined(__cplusplus)
CF_DIAGNOSTIC_PUSH()
CF_DIAGNOSTIC_IGNORE_CLANG("-Wgnu-anonymous-struct")
#endif

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

//-------------------------//
//   Fixed size integers   //
//-------------------------//

typedef uint8_t U8;
typedef uint16_t U16;
typedef uint32_t U32;
typedef uint64_t U64;

typedef int8_t I8;
typedef int16_t I16;
typedef int32_t I32;
typedef int64_t I64;

#define U8_MAX UINT8_MAX
#define I8_MIN INT8_MIN
#define I8_MAX INT8_MAX

#define U16_MAX UINT16_MAX
#define I16_MIN INT16_MIN
#define I16_MAX INT16_MAX

#define U32_MAX UINT32_MAX
#define I32_MIN INT32_MIN
#define I32_MAX INT32_MAX

#define U64_MAX UINT64_MAX
#define I64_MIN INT64_MIN
#define I64_MAX INT64_MAX

//----------------------------//
//   Size and pointer types   //
//----------------------------//

// Unsigned integer type of the result of sizeof, alignof and offsetof.
// Can be used as an unsigned representation of a pointer (ie. store an address).
typedef size_t Size;

// Signed integer type of the result of subtracting two pointers. Can be used
// as a memory offset.
typedef ptrdiff_t Offset;

#define OFFSET_MIN PTRDIFF_MIN
#define OFFSET_MAX PTRDIFF_MAX

CF_STATIC_ASSERT(sizeof(Size) == sizeof(uintptr_t), "Unexpeced uintptr_t size detected");
CF_STATIC_ASSERT(sizeof(Offset) == sizeof(intptr_t), "Unexpeced intptr_t size detected");

CF_STATIC_ASSERT(CF_PTR_SIZE == sizeof(void *), "Invalid pointer size detected");
CF_STATIC_ASSERT(CF_PTR_SIZE == sizeof(Size), "Invalid pointer size detected");
CF_STATIC_ASSERT(CF_PTR_SIZE == sizeof(Offset), "Invalid pointer size detected");

//---------------------//
//   Character types   //
//---------------------//

// NOTE (Matteo): This should be used instead of the integer equivalents to better communicate the
// fact that we are handling text

/// Standard 8-bits character (for ASCII and UTF8 strings)
typedef char Char8;

/// 16-bits "wide" character (for UTF16 strings)
typedef wchar_t Char16;

//--------------------------------------------------------//
//    Macros to retrieve min/max values for basic types   //
//--------------------------------------------------------//

// TODO (Matteo): Find a better name

// clang-format off
#define T_MIN(Type)        \
    _Generic((Type)(0),    \
             I8 : I8_MIN,  \
             I16: I16_MIN, \
             I32: I32_MIN, \
             I64: I64_MIN, \
             float: FLT_MIN, \
             double: DBL_MIN)

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
             float: FLT_MAX, \
             double: DBL_MAX)
// clang-format on

//----------------//
//   Error code   //
//----------------//

/// Error codes for IO operations, can be expanded for custom implementation
typedef U32 ErrorCode32;
enum
{
    Error_None = 0,

    Error_OutOfMemory,

    Error_OutOfRange,
    Error_BufferFull,
    Error_BufferEmpty,

    // IO operations
    Error_EndOfStream,
    Error_StreamTooLong,
    Error_FileError,

    /// Custom error codes must be greater than this one
    Error_Reserved,
};

//-----------------//
//   Time values   //
//-----------------//

/// Time duration, useful for performance measurement
typedef struct Duration
{
    I64 seconds;
    U32 nanos;
} Duration;

#define DURATION_INFINITE ((Duration){.nanos = U32_MAX})

/// Time representation used for interacting with OS APIs.
/// This type can be compared for ordering, but arithmetic is pointless
typedef U64 SystemTime;

//---------------//
//   Allocator   //
//---------------//

/// Definition of the main allocation function
#define MEM_ALLOCATOR_FN(name) \
    void *name(void *state, void *memory, Size old_size, Size new_size, Size align)

typedef MEM_ALLOCATOR_FN((*MemAllocatorFn));

/// Generic allocator interface
/// The memory provided by this interface should already be cleared to 0
typedef struct MemAllocator
{
    void *state;
    MemAllocatorFn func;
} MemAllocator;

//---------------------//
//   Dynamic buffers   //
//---------------------//

/// Macro to define a typed slice, represented as {pointer, size}
/// Can be used as an anonymous struct or member, or as a typedef for building a specific API.
#define MemSlice(Type)                              \
    struct                                          \
    {                                               \
        /* Pointer to actual storage */             \
        Type *ptr;                                  \
        /* Length of the slice (number of items) */ \
        Size len;                                   \
    }

/// Macro to define a typed buffer, represented as {pointer, size, capacity}
/// Can be used as an anonymous struct or member, or as a typedef for building a specific API.
/// Functionality is implemented in mem_buffer.inl, handling both fixed and dynamic capacity
#define MemBuffer(Type)                                                                           \
    struct                                                                                        \
    {                                                                                             \
        /* Slice that provides a pointer to the actual storage and the current size of the buffer \
         * (number of stored items) */                                                            \
        MemSlice(Type);                                                                           \
        /* Capacity of the buffer (number of elements that can be stored - acts as a watermark    \
         * for dynamic growth) */                                                                 \
        Size cap;                                                                                 \
    }

/// Slice of raw memory
typedef MemSlice(U8) MemRawSlice;

/// Buffer of raw memory, uses the same API as MemBuffer
typedef MemBuffer(U8) MemRawBuffer;

//-------------//
//   Strings   //
//-------------//

// TODO (Matteo): Naming review

/// Type alias for immutable, null-terminated C strings
typedef Char8 const *Cstr;

/// Immutable string slice/view. Not guaranteed to be null terminated.
/// Prefer it over C strings (safety, better API in progress)

typedef MemSlice(Char8 const) Str;

/// Dynamic string buffer
typedef struct StrBuilder
{
    MemAllocator alloc;
    // NOTE (Matteo): Including a dynamic array as an anonymous struct allows for
    // both extension and easy usage of the array API
    MemBuffer(Char8);
} StrBuilder;

/// Fixed size string buffer, useful for temporary string allocation or for storing
/// strings of limited size.
typedef struct StrBuffer
{
    Char8 data[1024];
    Str str;
} StrBuffer;

#define strValid(str) (!!(str).ptr)
#define strEnd(str) ((str).ptr + (str).len)

/// Build a string view from a string literal (static C string)
#define strLiteral(lit)                              \
    (Str)                                            \
    {                                                \
        .ptr = (lit), .len = CF_ARRAY_SIZE(lit) - 1, \
    }

//-------------//
//   Vectors   //
//-------------//

// TODO (Matteo): Review naming convention?

#define VEC_TYPES(Scalar, tag)    \
    /* 2D vector */               \
    typedef union Vec2##tag       \
    {                             \
        struct                    \
        {                         \
            Scalar x, y;          \
        };                        \
        struct                    \
        {                         \
            Scalar u, v;          \
        };                        \
        struct                    \
        {                         \
            Scalar width, height; \
        };                        \
        Scalar elem[2];           \
    } Vec2##tag;                  \
                                  \
    /* 3D vector */               \
    typedef union Vec3##tag       \
    {                             \
        struct                    \
        {                         \
            Scalar x, y, z;       \
        };                        \
        struct                    \
        {                         \
            Vec2##tag xy;         \
            Scalar _;             \
        };                        \
        Scalar elem[3];           \
    } Vec3##tag;                  \
                                  \
    /* 4D vector (quaternion) */  \
    typedef union Vec4##tag       \
    {                             \
        struct                    \
        {                         \
            Scalar x, y, z, w;    \
        };                        \
        struct                    \
        {                         \
            Vec3##tag xyz;        \
            Scalar _;             \
        };                        \
        struct                    \
        {                         \
            Vec2##tag xy;         \
            Vec2##tag zw;         \
        };                        \
        Scalar elem[4];           \
    } Vec4##tag;

VEC_TYPES(float, f)  // Vectors with single-precision float components
VEC_TYPES(double, d) // Vectors with double-precision float components
VEC_TYPES(I32, i)    // Vectors with (32 bit) integer components

#undef VEC_TYPES

// NOTE (Matteo): Matrix types use a column-major representation since it's the one
// used by GPU shaders, and so easy interop with OpenGL, Vulkan and D3D is ensured.

#define MAT_TYPE(Scalar, tag) \
    typedef union Mat4##tag   \
    {                         \
        Vec4##tag cols[4];    \
        Scalar elem[4][4];    \
        Scalar array[16];     \
    } Mat4##tag;

MAT_TYPE(float, f)  // Transformation matrix with single-precision float components
MAT_TYPE(double, d) // Transformation matrix with double-precision float components
MAT_TYPE(I32, i)    // Transformation matrix with (32 bit) integer components

#undef MAT_TYPE

//------------------//
//   Color spaces   //
//------------------//

/// Packed representation of a color in sRGB space with 8 bits per channel (including alpha)
typedef U32 Srgb32;

/// Represents a color in RGBA linear space as 4 floats in the [0,1] range
typedef union LinearColor
{
    struct
    {
        float r, g, b, a;
    };
    float channel[4];
} LinearColor;

/// Represents a color in HSV sRGB space, plus alpha channel, as 4 floats in the [0,1] range
typedef union HsvColor
{
    struct
    {
        float h, s, v, a;
    };
    float elem[4];
} HsvColor;

//----------------//
//   Rectangles   //
//----------------//

typedef union FRect
{
    struct
    {
        Vec2f p0, p1;
    };
    struct
    {
        float x0, y0, x1, y1;
    };
} FRect;

typedef union IRect
{
    struct
    {
        Vec2i p0, p1;
    };
    struct
    {
        I32 x0, y0, x1, y1;
    };
} IRect;

//-------------------//
//   Graphic types   //
//-------------------//

typedef struct ClipSpace
{
    float y_dir;
    float z_near;
    float z_far;
} ClipSpace;

//------------------------------------------------------------------------------

#if defined(__cplusplus)
CF_DIAGNOSTIC_POP()
#endif

//------------------------------------------------------------------------------

CF_DIAGNOSTIC_IGNORE_CLANG("-Wgnu-alignof-expression")
