#pragma once

#include "atom.h"

//----------------------------------------------------------------------------//
// Common read-write operations

#if ATOM__CLANG_BUILTINS
#    define atomInit(object, value) __c11_atomic_init(object, value)
#    define atomRead(object) __c11_atomic_load(object, __ATOMIC_RELAXED)
#    define atomWrite(object, value) __c11_atomic_store(object, value, __ATOMIC_RELAXED)
#else
// clang-format off

#define atomInit(object, value) (object)->inner = value

#define atomRead(object)                    \
    _Generic((object),                      \
             default : (object)->inner,     \
             AtomI64* : atomRead64(object), \
             AtomU64* : atomRead64(object))

#define atomWrite(object, value)                    \
    _Generic((object),                              \
             default : (object)->inner = value,     \
             AtomI64* : atomWrite64(object, value), \
             AtomU64* : atomWrite64(object, value))
// clang-format on
#endif // ATOM__CLANG_BUILTINS

//----------------------------------------------------------------------------//
// Platform specific built-ins

#if ATOM__CLANG_BUILTINS
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

// clang-format off

#define atomCompareExchange(value, expected, desired) \
    _Generic((value),                                 \
             AtomI8 * : atomCompareExchangeI8,        \
             AtomI16* : atomCompareExchangeI16,       \
             AtomI32* : atomCompareExchangeI32,       \
             AtomI64* : atomCompareExchangeI64,       \
             AtomU8 * : atomCompareExchangeU8,        \
             AtomU16* : atomCompareExchangeU16,       \
             AtomU32* : atomCompareExchangeU32,       \
             AtomU64* : atomCompareExchangeU64)(value, expected, desired)

// clang-format on

#    define atomCompareExchangeWeak(value, expected, desired)                          \
        __c11_atomic_compare_exchange_weak(value, expected, desired, __ATOMIC_RELAXED, \
                                           __ATOMIC_RELAXED)

#    define atomExchange(value, desired) __c11_atomic_exchange(value, desired, __ATOMIC_RELAXED)

#    define atomFetchAnd(value, operand) __c11_atomic_fetch_and(value, operand, __ATOMIC_RELAXED)

#    define atomFetchOr(value, operand) __c11_atomic_fetch_or(value, operand, __ATOMIC_RELAXED)

#    define atomFetchAdd(value, operand) __c11_atomic_fetch_add(value, operand, __ATOMIC_RELAXED)

#    define atomFetchSub(value, operand) __c11_atomic_fetch_sub(value, operand, __ATOMIC_RELAXED)

#    define atomFetchInc(value) atomFetchAdd(value, 1)

#    define atomFetchDec(value) atomFetchSub(value, 1)

#    define ATOM__COMPARE_EXCHANGE(Type)                                                      \
        static inline Type atomCompareExchange##Type(Atom##Type *object, Type expected,       \
                                                     Type desired)                            \
        {                                                                                     \
            Type got = expected;                                                              \
            if (__c11_atomic_compare_exchange_strong(object, &got, desired, __ATOMIC_RELAXED, \
                                                     __ATOMIC_RELAXED))                       \
            {                                                                                 \
                CF_ASSERT(got == expected, "");                                               \
            }                                                                                 \
            return got;                                                                       \
        }

ATOM__COMPARE_EXCHANGE(I8)
ATOM__COMPARE_EXCHANGE(I16)
ATOM__COMPARE_EXCHANGE(I32)
ATOM__COMPARE_EXCHANGE(I64)
ATOM__COMPARE_EXCHANGE(U8)
ATOM__COMPARE_EXCHANGE(U16)
ATOM__COMPARE_EXCHANGE(U32)
ATOM__COMPARE_EXCHANGE(U64)

// ATOM__CLANG_BUILTINS
//----------------------------------------------------------------------------//
#elif CF_OS_WIN32

void _ReadBarrier(void);
void _WriteBarrier(void);
void _ReadWriteBarrier(void);
void MemoryBarrier(void);

#    pragma intrinsic(_ReadBarrier, _WriteBarrier, _ReadWriteBarrier)

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

// clang-format off

#define atomCompareExchange(value, expected, desired) \
    _Generic((value),                                 \
             AtomI8 * : atomCompareExchangeI8,        \
             AtomI16* : atomCompareExchangeI16,       \
             AtomI32* : atomCompareExchangeI32,       \
             AtomI64* : atomCompareExchangeI64,       \
             AtomU8 * : atomCompareExchangeU8,        \
             AtomU16* : atomCompareExchangeU16,       \
             AtomU32* : atomCompareExchangeU32,       \
             AtomU64* : atomCompareExchangeU64)(value, expected, desired)

#define atomCompareExchangeWeak(value, expected, desired) \
    _Generic((value),                                 \
             AtomI8 * : atomCompareExchangeWeakI8,        \
             AtomI16* : atomCompareExchangeWeakI16,       \
             AtomI32* : atomCompareExchangeWeakI32,       \
             AtomI64* : atomCompareExchangeWeakI64,       \
             AtomU8 * : atomCompareExchangeWeakU8,        \
             AtomU16* : atomCompareExchangeWeakU16,       \
             AtomU32* : atomCompareExchangeWeakU32,       \
             AtomU64* : atomCompareExchangeWeakU64)(value, expected, desired)

#define atomExchange(value, desired) \
    _Generic((value),                                 \
             AtomI8 * : atomExchangeI8,        \
             AtomI16* : atomExchangeI16,       \
             AtomI32* : atomExchangeI32,       \
             AtomI64* : atomExchangeI64,       \
             AtomU8 * : atomExchangeU8,        \
             AtomU16* : atomExchangeU16,       \
             AtomU32* : atomExchangeU32,       \
             AtomU64* : atomExchangeU64)(value, desired)

#define atomFetchAnd(value, operand)                                          \
    _Generic((value),                                 \
             AtomI8 * : atomFetchAndI8,        \
             AtomI16* : atomFetchAndI16,       \
             AtomI32* : atomFetchAndI32,       \
             AtomI64* : atomFetchAndI64,       \
             AtomU8 * : atomFetchAndU8,        \
             AtomU16* : atomFetchAndU16,       \
             AtomU32* : atomFetchAndU32,       \
             AtomU64* : atomFetchAndU64)(value, operand)

#define atomFetchOr(value, operand)                                          \
    _Generic((value),                                 \
             AtomI8 * : atomFetchOrI8,        \
             AtomI16* : atomFetchOrI16,       \
             AtomI32* : atomFetchOrI32,       \
             AtomI64* : atomFetchOrI64,       \
             AtomU8 * : atomFetchOrU8,        \
             AtomU16* : atomFetchOrU16,       \
             AtomU32* : atomFetchOrU32,       \
             AtomU64* : atomFetchOrU64)(value, operand)

#define atomFetchAdd(value, operand)                                          \
    _Generic((value),                                 \
             AtomI8 * : atomFetchAddI8,        \
             AtomI16* : atomFetchAddI16,       \
             AtomI32* : atomFetchAddI32,       \
             AtomI64* : atomFetchAddI64,       \
             AtomU8 * : atomFetchAddU8,        \
             AtomU16* : atomFetchAddU16,       \
             AtomU32* : atomFetchAddU32,       \
             AtomU64* : atomFetchAddU64)(value, operand)

#define atomFetchSub(value, operand)                                          \
    _Generic((value),                                 \
             AtomI8 * : atomFetchSubI8,        \
             AtomI16* : atomFetchSubI16,       \
             AtomI32* : atomFetchSubI32,       \
             AtomI64* : atomFetchSubI64,       \
             AtomU8 * : atomFetchSubU8,        \
             AtomU16* : atomFetchSubU16,       \
             AtomU32* : atomFetchSubU32,       \
             AtomU64* : atomFetchSubU64)(value, operand)

#define atomFetchInc(value)                                          \
    _Generic((value),                                 \
             AtomI8 * : atomFetchIncI8,        \
             AtomI16* : atomFetchIncI16,       \
             AtomI32* : atomFetchIncI32,       \
             AtomI64* : atomFetchIncI64,       \
             AtomU8 * : atomFetchIncU8,        \
             AtomU16* : atomFetchIncU16,       \
             AtomU32* : atomFetchIncU32,       \
             AtomU64* : atomFetchIncU64)(value)

#define atomFetchDec(value)                                          \
    _Generic((value),                                 \
             AtomI8 * : atomFetchDecI8,        \
             AtomI16* : atomFetchDecI16,       \
             AtomI32* : atomFetchDecI32,       \
             AtomI64* : atomFetchDecI64,       \
             AtomU8 * : atomFetchDecU8,        \
             AtomU16* : atomFetchDecU16,       \
             AtomU32* : atomFetchDecU32,       \
             AtomU64* : atomFetchDecU64)(value)

// clang-format on

//--- CompareExchange  ---//

char _InterlockedCompareExchange8(char volatile *Destination, char Exchange, char Comparand);
short _InterlockedCompareExchange16(short volatile *Destination, short Exchange, short Comparand);
long _InterlockedCompareExchange(long volatile *Destination, long Exchange, long Comparand);
I64 _InterlockedCompareExchange64(I64 volatile *Destination, I64 Exchange, I64 Comparand);
void *_InterlockedCompareExchangePointer(void *volatile *Destination, void *Exchange,
                                         void *Comparand);

static inline void *
atomCompareExchangePtr(AtomPtr *object, void *expected, void *desired)
{
    return _InterlockedCompareExchangePointer((void *volatile *)object, desired, expected);
}

#    define ATOM__COMPARE_EXCHANGE(Type, SysType, suffix)                                          \
        static inline Type atomCompareExchange##Type(Atom##Type *object, Type expected,            \
                                                     Type desired)                                 \
        {                                                                                          \
            return (Type)_InterlockedCompareExchange##suffix((SysType volatile *)object,           \
                                                             (SysType)desired, (SysType)expected); \
        }                                                                                          \
                                                                                                   \
        static inline bool atomCompareExchangeWeak##Type(Atom##Type *object, Type *expected,       \
                                                         Type desired)                             \
        {                                                                                          \
            SysType exp_value = (SysType)(*(expected));                                            \
            SysType prev = _InterlockedCompareExchange##suffix((SysType volatile *)object,         \
                                                               (SysType)desired, exp_value);       \
            if (prev != exp_value)                                                                 \
            {                                                                                      \
                *expected = (Type)prev;                                                            \
                return false;                                                                      \
            }                                                                                      \
            return true;                                                                           \
        }

ATOM__COMPARE_EXCHANGE(I8, char, 8)
ATOM__COMPARE_EXCHANGE(I16, short, 16)
ATOM__COMPARE_EXCHANGE(I32, long, )
ATOM__COMPARE_EXCHANGE(I64, I64, 64)
ATOM__COMPARE_EXCHANGE(U8, char, 8)
ATOM__COMPARE_EXCHANGE(U16, short, 16)
ATOM__COMPARE_EXCHANGE(U32, long, )
ATOM__COMPARE_EXCHANGE(U64, I64, 64)

//--- Exchange  ---//

char _InterlockedExchange8(char volatile *Target, char Value);
short _InterlockedExchange16(short volatile *Target, short Value);
long _InterlockedExchange(long volatile *Target, long Value);
I64 _InterlockedExchange64(I64 volatile *Target, I64 Value);
void *_InterlockedExchangePointer(void *volatile *Target, void *Value);

#    pragma intrinsic(_InterlockedExchange8, _InterlockedExchange16, _InterlockedExchange, \
                      _InterlockedExchangePointer)

#    if CF_PTR_SIZE == 8
#        pragma intrinsic(_InterlockedExchange64)
#    endif

static inline void *
atomExchangePtr(AtomPtr *object, void *desired)
{
    return _InterlockedExchangePointer((void *volatile *)object, desired);
}

#    define ATOM__EXCHANGE(Type, SysType, suffix)                                 \
        static inline Type atomExchange##Type(Atom##Type *object, Type desired)   \
        {                                                                         \
            return (Type)_InterlockedExchange##suffix((SysType volatile *)object, \
                                                      (SysType)desired);          \
        }

ATOM__EXCHANGE(I8, char, 8)
ATOM__EXCHANGE(I16, short, 16)
ATOM__EXCHANGE(I32, long, )
ATOM__EXCHANGE(I64, I64, 64)
ATOM__EXCHANGE(U8, char, 8)
ATOM__EXCHANGE(U16, short, 16)
ATOM__EXCHANGE(U32, long, )
ATOM__EXCHANGE(U64, I64, 64)

//--- FetchAnd  ---//

char _InterlockedAnd8(char volatile *value, char mask);
short _InterlockedAnd16(short volatile *value, short mask);
long _InterlockedAnd(long volatile *value, long mask);
I64 _InterlockedAnd64(I64 volatile *value, I64 mask);

#    pragma intrinsic(_InterlockedAnd8, _InterlockedAnd16, _InterlockedAnd)

#    if CF_PTR_SIZE == 8
#        pragma intrinsic(_InterlockedAnd64)
#    endif

#    define ATOM__FETCH_AND(Type, SysType, suffix)                                             \
        static inline Type atomFetchAnd##Type(Atom##Type *value, Type operand)                 \
        {                                                                                      \
            return (Type)_InterlockedAnd##suffix((SysType volatile *)value, (SysType)operand); \
        }

ATOM__FETCH_AND(I8, char, 8)
ATOM__FETCH_AND(I16, short, 16)
ATOM__FETCH_AND(I32, long, )
ATOM__FETCH_AND(I64, I64, 64)
ATOM__FETCH_AND(U8, char, 8)
ATOM__FETCH_AND(U16, short, 16)
ATOM__FETCH_AND(U32, long, )
ATOM__FETCH_AND(U64, I64, 64)

//--- FetchOr  ---//

char _InterlockedOr8(char volatile *value, char mask);
short _InterlockedOr16(short volatile *value, short mask);
long _InterlockedOr(long volatile *value, long mask);
I64 _InterlockedOr64(I64 volatile *value, I64 mask);

#    pragma intrinsic(_InterlockedOr8, _InterlockedOr16, _InterlockedOr)

#    if CF_PTR_SIZE == 8
#        pragma intrinsic(_InterlockedOr64)
#    endif

#    define ATOM__FETCH_OR(Type, SysType, suffix)                                             \
        static inline Type atomFetchOr##Type(Atom##Type *value, Type operand)                 \
        {                                                                                     \
            return (Type)_InterlockedOr##suffix((SysType volatile *)value, (SysType)operand); \
        }

ATOM__FETCH_OR(I8, char, 8)
ATOM__FETCH_OR(I16, short, 16)
ATOM__FETCH_OR(I32, long, )
ATOM__FETCH_OR(I64, I64, 64)
ATOM__FETCH_OR(U8, char, 8)
ATOM__FETCH_OR(U16, short, 16)
ATOM__FETCH_OR(U32, long, )
ATOM__FETCH_OR(U64, I64, 64)

//--- FetchAdd  ---//

char _InterlockedExchangeAdd8(char volatile *Addend, char Value);
short _InterlockedExchangeAdd16(short volatile *Addend, short Value);
long _InterlockedExchangeAdd(long volatile *Addend, long Value);
I64 _InterlockedExchangeAdd64(I64 volatile *Addend, I64 Value);

#    pragma intrinsic(_InterlockedExchangeAdd8, _InterlockedExchangeAdd16, _InterlockedExchangeAdd)

#    if CF_PTR_SIZE == 8
#        pragma intrinsic(_InterlockedExchangeAdd64)
#    endif

#    define ATOM__FETCH_ADD(Type, SysType, suffix)                                  \
        static inline Type atomFetchAdd##Type(Atom##Type *value, Type operand)      \
        {                                                                           \
            return (Type)_InterlockedExchangeAdd##suffix((SysType volatile *)value, \
                                                         (SysType)operand);         \
        }

ATOM__FETCH_ADD(I8, char, 8)
ATOM__FETCH_ADD(I16, short, 16)
ATOM__FETCH_ADD(I32, long, )
ATOM__FETCH_ADD(I64, I64, 64)
ATOM__FETCH_ADD(U8, char, 8)
ATOM__FETCH_ADD(U16, short, 16)
ATOM__FETCH_ADD(U32, long, )
ATOM__FETCH_ADD(U64, I64, 64)

//--- FetchSub  ---//

#    define ATOM__FETCH_SUB(Type, SysType, suffix)                                  \
        static inline Type atomFetchSub##Type(Atom##Type *value, Type operand)      \
        {                                                                           \
            return (Type)_InterlockedExchangeAdd##suffix((SysType volatile *)value, \
                                                         -(SysType)(operand));      \
        }

ATOM__FETCH_SUB(I8, char, 8)
ATOM__FETCH_SUB(I16, short, 16)
ATOM__FETCH_SUB(I32, long, )
ATOM__FETCH_SUB(I64, I64, 64)
ATOM__FETCH_SUB(U8, char, 8)
ATOM__FETCH_SUB(U16, short, 16)
ATOM__FETCH_SUB(U32, long, )
ATOM__FETCH_SUB(U64, I64, 64)

//--- FetchInc  ---//

short _InterlockedIncrement16(short volatile *lpAddend);
long _InterlockedIncrement(long volatile *lpAddend);
I64 _InterlockedIncrement64(I64 volatile *lpAddend);

#    pragma intrinsic(_InterlockedIncrement16, _InterlockedIncrement)

#    if CF_PTR_SIZE == 8
#        pragma intrinsic(_InterlockedIncrement64)
#    endif

static inline I8
atomFetchIncI8(AtomI8 *object)
{
    char volatile *value = (char volatile *)object;
    char prev;

    do
    {
        prev = *value;
    } while (_InterlockedCompareExchange8((char volatile *)value, prev + 1, prev) != prev);

    return (I8)prev;
}

static inline U8
atomFetchIncU8(AtomU8 *object)
{
    char volatile *value = (char volatile *)object;
    char prev;

    do
    {
        prev = *value;
    } while (_InterlockedCompareExchange8((char volatile *)value, prev + 1, prev) != prev);

    return (U8)prev;
}

#    define ATOM__FETCH_INC(Type, SysType, suffix)                                     \
        static inline Type atomFetchInc##Type(Atom##Type *value)                       \
        {                                                                              \
            return (Type)_InterlockedIncrement##suffix((SysType volatile *)value) - 1; \
        }

ATOM__FETCH_INC(I16, short, 16)
ATOM__FETCH_INC(I32, long, )
ATOM__FETCH_INC(I64, I64, 64)
ATOM__FETCH_INC(U16, short, 16)
ATOM__FETCH_INC(U32, long, )
ATOM__FETCH_INC(U64, I64, 64)

//--- FetchDec  ---//

short _InterlockedDecrement16(short volatile *lpAddend);
long _InterlockedDecrement(long volatile *lpAddend);
I64 _InterlockedDecrement64(I64 volatile *lpAddend);

#    pragma intrinsic(_InterlockedDecrement16, _InterlockedDecrement)

#    if CF_PTR_SIZE == 8
#        pragma intrinsic(_InterlockedDecrement64)
#    endif

static inline I8
atomFetchDecI8(AtomI8 *object)
{
    char volatile *value = (char volatile *)object;
    char prev;

    do
    {
        prev = *value;
    } while (_InterlockedCompareExchange8(value, prev - 1, prev) != prev);

    return (I8)prev;
}

static inline U8
atomFetchDecU8(AtomU8 *object)
{
    char volatile *value = (char volatile *)object;
    char prev;

    do
    {
        prev = *value;
    } while (_InterlockedCompareExchange8(value, prev - 1, prev) != prev);

    return (U8)prev;
}

#    define ATOM__FETCH_DEC(Type, SysType, suffix)                                     \
        static inline Type atomFetchDec##Type(Atom##Type *value)                       \
        {                                                                              \
            return (Type)_InterlockedDecrement##suffix((SysType volatile *)value) + 1; \
        }

ATOM__FETCH_DEC(I16, short, 16)
ATOM__FETCH_DEC(I32, long, )
ATOM__FETCH_DEC(I64, I64, 64)
ATOM__FETCH_DEC(U16, short, 16)
ATOM__FETCH_DEC(U32, long, )
ATOM__FETCH_DEC(U64, I64, 64)

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
