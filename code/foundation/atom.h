#pragma once

#include "core.h"

#if CF_COMPILER_MSVC
#    define ATOM_ENSURE_ALIGN(decl, amt) __declspec(align(amt)) decl
#else
#    define ATOM_ENSURE_ALIGN(decl, amt) decl __attribute__((aligned(amt)))
#endif

#if CF_COMPILER_CLANG && __has_builtin(__c11_atomic_init)
#    define ATOM__CLANG_BUILTIN 1
#else
#    define ATOM__CLANG_BUILTIN 0
#endif

//----------------------------------------------------------------------------//
// Types

#if ATOM__CLANG_BUILTIN

typedef _Atomic(I8) AtomI8;
typedef _Atomic(U8) AtomU8;
typedef _Atomic(I16) AtomI16;
typedef _Atomic(U16) AtomU16;
typedef _Atomic(I32) AtomI32;
typedef _Atomic(U32) AtomU32;
typedef _Atomic(I64) AtomI64;
typedef _Atomic(U64) AtomU64;
typedef _Atomic(Isize) AtomIsize;
typedef _Atomic(Usize) AtomUsize;
typedef _Atomic(void *) AtomPtr;

#else

// clang-format off
ATOM_ENSURE_ALIGN(typedef struct AtomI32 { I32  volatile  inner; } AtomI32, 4);
                  typedef struct AtomI8  { I8   volatile  inner; } AtomI8;
ATOM_ENSURE_ALIGN(typedef struct AtomI16 { I16  volatile  inner; } AtomI16, 2);
ATOM_ENSURE_ALIGN(typedef struct AtomI64 { I64  volatile  inner; } AtomI64, 8);
                  typedef struct AtomU8  { U8   volatile  inner; } AtomU8;
ATOM_ENSURE_ALIGN(typedef struct AtomU16 { U16  volatile  inner; } AtomU16, 2);
ATOM_ENSURE_ALIGN(typedef struct AtomU32 { U32  volatile  inner; } AtomU32, 4);
ATOM_ENSURE_ALIGN(typedef struct AtomU64 { U64  volatile  inner; } AtomU64, 8);
ATOM_ENSURE_ALIGN(typedef struct AtomPtr { void volatile *inner; } AtomPtr, CF_PTR_SIZE);
// clang-format on

#    if CF_PTR_SIZE == 4

CF_STATIC_ASSERT(sizeof(Isize) == 4 && sizeof(Usize) == 4, "Invalid size types");

typedef AtomI32 AtomIsize;
typedef AtomU32 AtomUsize;

#    elif CF_PTR_SIZE == 8

CF_STATIC_ASSERT(sizeof(Isize) == 8 && sizeof(Usize) == 8, "Invalid size types");

typedef AtomI64 AtomIsize;
typedef AtomU64 AtomUsize;

#    else
#        error "Unsupported pointer size"
#    endif

#endif

//----------------------------------------------------------------------------//
// Common read-write operations

#if ATOM__CLANG_BUILTIN
#    define atomRead(value) __c11_atomic_load(value, __ATOMIC_RELAXED)
#    define atomWrite(object, value) __c11_atomic_store(object, value, __ATOMIC_RELAXED)
#else
// clang-format off
#define atomRead(x)                     \
    _Generic((x),                       \
             AtomI8 * : (x)->inner,     \
             AtomI16* : (x)->inner,     \
             AtomI32* : (x)->inner,     \
             AtomI64* : atomRead64(x),  \
             AtomU8 * : (x)->inner,     \
             AtomU16* : (x)->inner,     \
             AtomU32* : (x)->inner,     \
             AtomU64* : atomRead64(x),  \
             AtomPtr* : (x)->inner)

#define atomWrite(x, y)                     \
    _Generic((x),                           \
             AtomI8 * : (x)->inner = y,     \
             AtomI16* : (x)->inner = y,     \
             AtomI32* : (x)->inner = y,     \
             AtomI64* : atomWrite64(x, y),  \
             AtomU16* : (x)->inner = y,     \
             AtomU32* : (x)->inner = y,     \
             AtomU8 * : (x)->inner = y,     \
             AtomU64* : atomWrite64(x, y),  \
             AtomPtr* : (x)->inner = y)
// clang-format on
#endif // ATOM__CLANG_BUILTIN

//----------------------------------------------------------------------------//
// Platform specific built-ins

#if ATOM__CLANG_BUILTIN
//-----------------------//
//   CPU memory fences   //
//-----------------------//

#    define atomAcquireFence() __c11_atomic_thread_fence(__ATOMIC_ACQUIRE)
#    define atomReleaseFence() __c11_atomic_thread_fence(__ATOMIC_RELEASE)
#    define atomSequentialFence() __c11_atomic_thread_fence(__ATOMIC_SEQ_CST)

//---------------------//
//   Compiler fences   //
//---------------------//

#    define atomAcquireCompFence() __c11_atomic_signal_fence(__ATOMIC_ACQUIRE)
#    define atomReleaseCompFence() __c11_atomic_signal_fence(__ATOMIC_RELEASE)
#    define atomSequentialCompFence() __c11_atomic_signal_fence(__ATOMIC_SEQ_CST)

//-----------------------//
//   Atomic operations   //
//-----------------------//

#    define atomCompareExchange(value, expected, desired)                                \
        __c11_atomic_compare_exchange_strong(value, expected, desired, __ATOMIC_RELAXED, \
                                             __ATOMIC_RELAXED)

#    define atomExchange(value, desired) __c11_atomic_exchange(value, desired, __ATOMIC_RELAXED)

#    define atomFetchAnd(value, operand) __c11_atomic_fetch_and(value, operand, __ATOMIC_RELAXED)

#    define atomFetchOr(value, operand) __c11_atomic_fetch_or(value, operand, __ATOMIC_RELAXED)

#    define atomFetchAdd(value, operand) __c11_atomic_fetch_add(value, operand, __ATOMIC_RELAXED)

#    define atomFetchSub(value, operand) __c11_atomic_fetch_sub(value, operand, __ATOMIC_RELAXED)

#    define atomFetchInc(value) atomFetchAdd(value, 1)

#    define atomFetchDec(value) atomFetchSub(value, 1)

// ATOM__CLANG_BUILTIN
//----------------------------------------------------------------------------//
#elif CF_OS_WIN32

#    include <intrin.h>

#    pragma intrinsic(                                                                             \
        _ReadBarrier, _WriteBarrier, _ReadWriteBarrier, _InterlockedCompareExchange8,              \
        _InterlockedCompareExchange16, _InterlockedCompareExchange, _InterlockedCompareExchange64, \
        _InterlockedCompareExchangePointer, _InterlockedExchange8, _InterlockedExchange16,         \
        _InterlockedExchange, _InterlockedExchange64, _InterlockedExchangePointer,                 \
        _InterlockedExchangeAdd8, _InterlockedExchangeAdd16, _InterlockedExchangeAdd,              \
        _InterlockedExchangeAdd64, _InterlockedIncrement16, _InterlockedIncrement,                 \
        _InterlockedIncrement64, _InterlockedDecrement16, _InterlockedDecrement,                   \
        _InterlockedDecrement64, _InterlockedAnd8, _InterlockedAnd16, _InterlockedAnd,             \
        _InterlockedAnd64, _InterlockedOr8, _InterlockedOr16, _InterlockedOr, _InterlockedOr64)

#    if CF_COMPILER_CLANG

#        define ATOM__UNDEPRECATE(intrin)                                         \
            _Pragma("clang diagnostic push")                                      \
                _Pragma("clang diagnostic ignored \"-Wdeprecated-declarations\"") \
                    intrin _Pragma("clang diagnostic pop")
#    else
#        define ATOM__UNDEPRECATE(intrin)
#    endif

//-----------------------//
//   CPU memory fences   //
//-----------------------//

#    define atomAcquireFence() ATOM__UNDEPRECATE(_ReadBarrier())
#    define atomReleaseFence() ATOM__UNDEPRECATE(_WriteBarrier())
#    define atomSequentialFence() ATOM__UNDEPRECATE(MemoryBarrier())

//---------------------//
//   Compiler fences   //
//---------------------//

#    define atomAcquireCompFence() ATOM__UNDEPRECATE(_ReadBarrier())
#    define atomReleaseCompFence() ATOM__UNDEPRECATE(_WriteBarrier())
#    define atomSequentialCompFence() ATOM__UNDEPRECATE(_ReadWriteBarrier())

//-----------------------//
//   Atomic operations   //
//-----------------------//

// TODO (Matteo): Restore pointer operations

// clang-format off

#define atomCompareExchange(value, expected, desired)                                                   \
    _Generic((value),                                                                                   \
             AtomI8 * : _InterlockedCompareExchange8      ((char volatile*)(value), desired, expected), \
             AtomI16* : _InterlockedCompareExchange16     ((I16  volatile*)(value), desired, expected), \
             AtomI32* : _InterlockedCompareExchange       ((long volatile*)(value), desired, expected), \
             AtomI64* : _InterlockedCompareExchange64     ((I64  volatile*)(value), desired, expected), \
             AtomU8 * : _InterlockedCompareExchange8      ((char volatile*)(value), desired, expected), \
             AtomU16* : _InterlockedCompareExchange16     ((I16  volatile*)(value), desired, expected), \
             AtomU32* : _InterlockedCompareExchange       ((long volatile*)(value), desired, expected), \
             AtomU64* : _InterlockedCompareExchange64     ((I64  volatile*)(value), desired, expected))


#define atomExchange(value, desired)                                                    \
    _Generic((value),                                                                   \
             AtomI8 * : _InterlockedExchange8      ((char volatile *)(value), desired), \
             AtomI16* : _InterlockedExchange16     ((I16  volatile *)(value), desired), \
             AtomI32* : _InterlockedExchange       ((long volatile *)(value), desired), \
             AtomI64* : _InterlockedExchange64     ((I64  volatile *)(value), desired), \
             AtomU8 * : _InterlockedExchange8      ((char volatile *)(value), desired), \
             AtomU16* : _InterlockedExchange16     ((I16  volatile *)(value), desired), \
             AtomU32* : _InterlockedExchange       ((long volatile *)(value), desired), \
             AtomU64* : _InterlockedExchange64     ((I64  volatile *)(value), desired))

#define atomFetchAnd(value, operand)                                          \
    _Generic((value),                                                         \
             AtomI8 * : _InterlockedAnd8 ((char volatile *)(value), operand), \
             AtomI16* : _InterlockedAnd16((I16  volatile *)(value), operand), \
             AtomI32* : _InterlockedAnd  ((long volatile *)(value), operand), \
             AtomI64* : _InterlockedAnd64((I64  volatile *)(value), operand), \
             AtomU8 * : _InterlockedAnd8 ((char volatile *)(value), operand), \
             AtomU16* : _InterlockedAnd16((I16  volatile *)(value), operand), \
             AtomU32* : _InterlockedAnd  ((long volatile *)(value), operand), \
             AtomU64* : _InterlockedAnd64((I64  volatile *)(value), operand))

#define atomFetchOr(value, operand)                                          \
    _Generic((value),                                                        \
             AtomI8 * : _InterlockedOr8 ((char volatile *)(value), operand), \
             AtomI16* : _InterlockedOr16((I16  volatile *)(value), operand), \
             AtomI32* : _InterlockedOr  ((long volatile *)(value), operand), \
             AtomI64* : _InterlockedOr64((I64  volatile *)(value), operand), \
             AtomU8 * : _InterlockedOr8 ((char volatile *)(value), operand), \
             AtomU16* : _InterlockedOr16((I16  volatile *)(value), operand), \
             AtomU32* : _InterlockedOr  ((long volatile *)(value), operand), \
             AtomU64* : _InterlockedOr64((I64  volatile *)(value), operand))

#define atomFetchAdd(value, operand)                                                 \
    _Generic((value),                                                                \
             AtomI8 * : _InterlockedExchangeAdd8 ((char volatile*)(value), operand), \
             AtomI16* : _InterlockedExchangeAdd16((I16  volatile*)(value), operand), \
             AtomI32* : _InterlockedExchangeAdd  ((long volatile*)(value), operand), \
             AtomI64* : _InterlockedExchangeAdd64((I64  volatile*)(value), operand), \
             AtomU8 * : _InterlockedExchangeAdd8 ((char volatile*)(value), operand), \
             AtomU16* : _InterlockedExchangeAdd16((I16  volatile*)(value), operand), \
             AtomU32* : _InterlockedExchangeAdd  ((long volatile*)(value), operand), \
             AtomU64* : _InterlockedExchangeAdd64((I64  volatile*)(value), operand))

#define atomFetchSub(value, operand)                                                 \
    _Generic((value),                                                                \
             AtomI8 * : _InterlockedExchangeAdd8 ((char volatile*)(value), -(char)(operand)), \
             AtomI16* : _InterlockedExchangeAdd16((I16  volatile*)(value), -(I16 )(operand)), \
             AtomI32* : _InterlockedExchangeAdd  ((long volatile*)(value), -(long)(operand)), \
             AtomI64* : _InterlockedExchangeAdd64((I64  volatile*)(value), -(I64 )(operand)), \
             AtomU8 * : _InterlockedExchangeAdd8 ((char volatile*)(value), -(char)(operand)), \
             AtomU16* : _InterlockedExchangeAdd16((I16  volatile*)(value), -(I16 )(operand)), \
             AtomU32* : _InterlockedExchangeAdd  ((long volatile*)(value), -(long)(operand)), \
             AtomU64* : _InterlockedExchangeAdd64((I64  volatile*)(value), -(I64 )(operand)))

#define atomFetchInc(value)                                                     \
    _Generic((value),                                                           \
             AtomI8 * : (U8)atom__FetchInc8     ((char volatile*)(value)),      \
             AtomI16* : (_InterlockedIncrement16((I16  volatile*)(value)) - 1), \
             AtomI32* : (_InterlockedIncrement  ((long volatile*)(value)) - 1), \
             AtomI64* : (_InterlockedIncrement64((I64  volatile*)(value)) - 1), \
             AtomU8 * : (I8)atom__FetchInc8     ((char volatile*)(value)),      \
             AtomU16* : (_InterlockedIncrement16((I16  volatile*)(value)) - 1), \
             AtomU32* : (_InterlockedIncrement  ((long volatile*)(value)) - 1), \
             AtomU64* : (_InterlockedIncrement64((I64  volatile*)(value)) - 1))

#define atomFetchDec(value)                                                      \
    _Generic((value),                                                            \
             AtomI8 * : (U8)atom__FetchDec8     ((char volatile*)(value)),      \
             AtomI16* : (_InterlockedDecrement16((I16  volatile*)(value)) + 1), \
             AtomI32* : (_InterlockedDecrement  ((long volatile*)(value)) + 1), \
             AtomI64* : (_InterlockedDecrement64((I64  volatile*)(value)) + 1), \
             AtomU8 * : (I8)atom__FetchDec8     ((char volatile*)(value)),      \
             AtomU16* : (_InterlockedDecrement16((I16  volatile*)(value)) + 1), \
             AtomU32* : (_InterlockedDecrement  ((long volatile*)(value)) + 1), \
             AtomU64* : (_InterlockedDecrement64((I64  volatile*)(value)) + 1))

// clang-format on

//---------------------------//
//  8bit specializations     //
//---------------------------//

static inline char
atom__FetchInc8(char volatile *value)
{
    char prev;

    do
    {
        prev = *value;
    } while (_InterlockedCompareExchange8(value, prev + 1, prev) != prev);

    return prev;
}
static inline char
atom__FetchDec8(char volatile *value)
{
    char prev;

    do
    {
        prev = *value;
    } while (_InterlockedCompareExchange8(value, prev - 1, prev) != prev);

    return prev;
}

//---------------------------//
//   64bit specializations   //
//---------------------------//

#    if (CF_PTR_SIZE == 8)
#        define atomRead64(x) (x)->inner
#        define atomWrite64(x, y) (x)->inner = (y)
#    else
#        define atomRead64(x) atom__Read64((U64 volatile *)(x))
#        define atomWrite64(x, y) atom__Write64((U64 volatile *)(x), (U64)(y))

static inline U64
atom__Read64(U64 volatile *object)
{
    // On 32-bit x86, the most compatible way to get an atomic 64-bit load is with
    // cmpxchg8b. This essentially performs atomCompareExchange(object, _dummyValue,
    // _dummyValue).
    U64 result;
    __asm {
        mov esi, object;
        mov ebx, eax;
        mov ecx, edx;
        lock cmpxchg8b [esi];
        mov dword ptr result, eax;
        mov dword ptr result[4], edx;
    }
    return result;
}

static inline void
atom__Write64(U64 volatile *object, U64 value)
{
    // On 32-bit x86, the most compatible way to get an atomic 64-bit store is with
    // cmpxchg8b. Essentially, we perform atomCompareExchange(object, object->inner,
    // desired) in a loop until it returns the previous value. According to the Linux
    // kernel (atomic64_cx8_32.S), we don't need the "lock;" prefix on cmpxchg8b since
    // aligned 64-bit writes are already atomic on 586 and newer.
    __asm {
        mov esi, object;
        mov ebx, dword ptr value;
        mov ecx, dword ptr value[4];
    retry:
        cmpxchg8b [esi];
        jne retry;
    }
}

#    endif

// CF_OS_WIN32
//----------------------------------------------------------------------------//
#else
#    error "Atomics not yet supported on this platform"
#endif
