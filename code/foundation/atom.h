#pragma once

#include "core.h"

#if CF_ARCH_X86
#    define ATOM_PTR_SIZE 4
#elif CF_ARCH_X64
#    define ATOM_PTR_SIZE 8
#else
#    error "Architecture not detected"
#endif

CF_STATIC_ASSERT(sizeof(void *) == ATOM_PTR_SIZE, "Invalid pointer size detected");

#if CF_COMPILER_MSVC
#    define ATOM_ENSURE_ALIGN(decl, amt) __declspec(align(amt)) decl
#else
#    define ATOM_ENSURE_ALIGN(decl, amt) decl __attribute__((aligned(amt)))
#endif

// clang-format off
                  typedef struct AtomI8  { I8   volatile  inner; } AtomI8;
ATOM_ENSURE_ALIGN(typedef struct AtomI16 { I16  volatile  inner; } AtomI16, 2);
ATOM_ENSURE_ALIGN(typedef struct AtomI32 { I32  volatile  inner; } AtomI32, 4);
ATOM_ENSURE_ALIGN(typedef struct AtomI64 { I64  volatile  inner; } AtomI64, 8);
                  typedef struct AtomU8  { U8   volatile  inner; } AtomU8;
ATOM_ENSURE_ALIGN(typedef struct AtomU16 { U16  volatile  inner; } AtomU16, 2);
ATOM_ENSURE_ALIGN(typedef struct AtomU32 { U32  volatile  inner; } AtomU32, 4);
ATOM_ENSURE_ALIGN(typedef struct AtomU64 { U64  volatile  inner; } AtomU64, 8);
ATOM_ENSURE_ALIGN(typedef struct AtomPtr { void volatile *inner; } AtomPtr, ATOM_PTR_SIZE);
// clang-format on

//----------------------------------------------------------------------------//

// clang-format off
#define atomRead(x)                     \
    _Generic((x),                       \
             AtomI8 * : (x)->inner,     \
             AtomI16* : (x)->inner,     \
             AtomI32* : (x)->inner,     \
             AtomI64* : atomReadI64(x), \
             AtomU8 * : (x)->inner,     \
             AtomU16* : (x)->inner,     \
             AtomU32* : (x)->inner,     \
             AtomU64* : atomReadU64(x), \
             AtomPtr* : (x)->inner)

#define atomWrite(x, y)                     \
    _Generic((x),                           \
             AtomI8 * : (x)->inner = y,     \
             AtomI16* : (x)->inner = y,     \
             AtomI32* : (x)->inner = y,     \
             AtomI64* : atomWriteI64(x, y), \
             AtomU16* : (x)->inner = y,     \
             AtomU32* : (x)->inner = y,     \
             AtomU8 * : (x)->inner = y,     \
             AtomU64* : atomWriteU64(x, y), \
             AtomPtr* : (x)->inner = y)
// clang-format on

//----------------------------------------------------------------------------//

#if CF_OS_WIN32

#    include <intrin.h>

//------------//
//   Fences   //
//------------//

#    define atomSignalFenceAcquire() _ReadWriteBarrier()
#    define atomSignalFenceRelease() _ReadWriteBarrier()
#    define atomSignalFenceSequential() _ReadWriteBarrier()

#    define atomThreadFenceAcquire() _ReadWriteBarrier()
#    define atomThreadFenceRelease() _ReadWriteBarrier()
#    define atomThreadFenceSequential() MemoryBarrier()

//-----------------------//
//   Atomic operations   //
//-----------------------//

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
             AtomU64* : _InterlockedCompareExchange64     ((I64  volatile*)(value), desired, expected), \
             AtomPtr* : _InterlockedCompareExchangePointer((void volatile*)(value), desired, expected))

#define atomExchange(value, desired)                                                    \
    _Generic((value),                                                                   \
             AtomI8 * : _InterlockedExchange8      ((char volatile *)(value), desired), \
             AtomI16* : _InterlockedExchange16     ((I16  volatile *)(value), desired), \
             AtomI32* : _InterlockedExchange       ((long volatile *)(value), desired), \
             AtomI64* : _InterlockedExchange64     ((I64  volatile *)(value), desired), \
             AtomU8 * : _InterlockedExchange8      ((char volatile *)(value), desired), \
             AtomU16* : _InterlockedExchange16     ((I16  volatile *)(value), desired), \
             AtomU32* : _InterlockedExchange       ((long volatile *)(value), desired), \
             AtomU64* : _InterlockedExchange64     ((I64  volatile *)(value), desired), \
             AtomPtr* : _InterlockedExchangePointer((void volatile *)(value), desired))

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
             AtomI8 * : _InterlockedExchangeAdd8 ((volatile char*)(value), operand), \
             AtomI16* : _InterlockedExchangeAdd16((volatile I16 *)(value), operand), \
             AtomI32* : _InterlockedExchangeAdd  ((volatile long*)(value), operand), \
             AtomI64* : _InterlockedExchangeAdd64((volatile I64 *)(value), operand), \
             AtomU8 * : _InterlockedExchangeAdd8 ((volatile char*)(value), operand), \
             AtomU16* : _InterlockedExchangeAdd16((volatile I16 *)(value), operand), \
             AtomU32* : _InterlockedExchangeAdd  ((volatile long*)(value), operand), \
             AtomU64* : _InterlockedExchangeAdd64((volatile I64 *)(value), operand))

#define atomFetchSub(value, operand)                                                 \
    _Generic((value),                                                                \
             AtomI8 * : _InterlockedExchangeSub8 ((volatile char*)(value), operand), \
             AtomI16* : _InterlockedExchangeSub16((volatile I16 *)(value), operand), \
             AtomI32* : _InterlockedExchangeSub  ((volatile long*)(value), operand), \
             AtomI64* : _InterlockedExchangeSub64((volatile I64 *)(value), operand), \
             AtomU8 * : _InterlockedExchangeSub8 ((volatile char*)(value), operand), \
             AtomU16* : _InterlockedExchangeSub16((volatile I16 *)(value), operand), \
             AtomU32* : _InterlockedExchangeSub  ((volatile long*)(value), operand), \
             AtomU64* : _InterlockedExchangeSub64((volatile I64 *)(value), operand))

#define atomIncrement(value)                                              \
    _Generic((value),                                                     \
             AtomI8 * : _InterlockedIncrement8 ((volatile char*)(value)), \
             AtomI16* : _InterlockedIncrement16((volatile I16 *)(value)), \
             AtomI32* : _InterlockedIncrement  ((volatile long*)(value)), \
             AtomI64* : _InterlockedIncrement64((volatile I64 *)(value)), \
             AtomU8 * : _InterlockedIncrement8 ((volatile char*)(value)), \
             AtomU16* : _InterlockedIncrement16((volatile I16 *)(value)), \
             AtomU32* : _InterlockedIncrement  ((volatile long*)(value)), \
             AtomU64* : _InterlockedIncrement64((volatile I64 *)(value)))

#define atomDecrement(value)                                              \
    _Generic((value),                                                     \
             AtomI8 * : _InterlockedDecrement8 ((volatile char*)(value)), \
             AtomI16* : _InterlockedDecrement16((volatile I16 *)(value)), \
             AtomI32* : _InterlockedDecrement  ((volatile long*)(value)), \
             AtomI64* : _InterlockedDecrement64((volatile I64 *)(value)), \
             AtomU8 * : _InterlockedDecrement8 ((volatile char*)(value)), \
             AtomU16* : _InterlockedDecrement16((volatile I16 *)(value)), \
             AtomU32* : _InterlockedDecrement  ((volatile long*)(value)), \
             AtomU64* : _InterlockedDecrement64((volatile I64 *)(value)))

// clang-format on

//---------------------------//
//   64bit specializations   //
//---------------------------//

static inline I64
atomReadI64(AtomI64 const *object)
{
#    if (ATOM_PTR_SIZE == 8)
    return object->inner;
#    else
    // On 32-bit x86, the most compatible way to get an atomic 64-bit load is with cmpxchg8b.
    // This essentially performs atomCompareExchange(object, _dummyValue, _dummyValue).
    I64 result;
    __asm {
        mov esi, object;
        mov ebx, eax;
        mov ecx, edx;
        lock cmpxchg8b [esi];
        mov dword ptr result, eax;
        mov dword ptr result[4], edx;
    }
    return result;
#    endif
}

static inline U64
atomReadU64(AtomU64 const *object)
{
#    if (ATOM_PTR_SIZE == 8)
    return object->inner;
#    else
    // On 32-bit x86, the most compatible way to get an atomic 64-bit load is with cmpxchg8b.
    // This essentially performs atomCompareExchange(object, _dummyValue, _dummyValue).
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
#    endif
}

static inline void
atomWriteU64(AtomU64 *object, U64 value)
{
#    if (CF_ARCH_X64)
    object->inner = value;
#    else
    // On 32-bit x86, the most compatible way to get an atomic 64-bit store is with cmpxchg8b.
    // Essentially, we perform atomCompareExchange(object, object->inner, desired)
    // in a loop until it returns the previous value.
    // According to the Linux kernel (atomic64_cx8_32.S), we don't need the "lock;" prefix
    // on cmpxchg8b since aligned 64-bit writes are already atomic on 586 and newer.
    __asm {
        mov esi, object;
        mov ebx, dword ptr value;
        mov ecx, dword ptr value[4];
    retry:
        cmpxchg8b [esi];
        jne retry;
    }
#    endif
}

static inline void
atomWriteI64(AtomI64 *object, I64 value)
{
#    if (CF_ARCH_X64)
    object->inner = value;
#    else
    // On 32-bit x86, the most compatible way to get an atomic 64-bit store is with cmpxchg8b.
    // Essentially, we perform atomCompareExchange(object, object->inner, desired)
    // in a loop until it returns the previous value.
    // According to the Linux kernel (atomic64_cx8_32.S), we don't need the "lock;" prefix
    // on cmpxchg8b since aligned 64-bit writes are already atomic on 586 and newer.
    __asm {
        mov esi, object;
        mov ebx, dword ptr value;
        mov ecx, dword ptr value[4];
    retry:
        cmpxchg8b [esi];
        jne retry;
    }
#    endif
}

//----------------------------------------------------------------------------//

#else
#    error "Atomics not yet supported on this platform"
#endif
