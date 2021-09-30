#pragma once

#include "core.h"

// Atomic Types

#if CF_COMPILER_CLANG
#    if __has_builtin(__c11_atomic_init)
#        define ATOM__CLANG_BUILTINS 1
#    endif
#endif

#if !defined(ATOM__CLANG_BUILTINS)
#    define ATOM__CLANG_BUILTINS 0
#endif

#if ATOM__CLANG_BUILTINS
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
typedef _Atomic(bool) AtomBool;
#else
#    if CF_COMPILER_MSVC
#        define ATOM__ALIGN(decl, amt) __declspec(align(amt)) decl
#    else
#        define ATOM__ALIGN(decl, amt) decl __attribute__((aligned(amt)))
#    endif

// clang-format off
ATOM__ALIGN(typedef struct AtomI32  { I32  volatile  inner; } AtomI32, 4);
            typedef struct AtomI8   { I8   volatile  inner; } AtomI8;
ATOM__ALIGN(typedef struct AtomI16  { I16  volatile  inner; } AtomI16, 2);
ATOM__ALIGN(typedef struct AtomI64  { I64  volatile  inner; } AtomI64, 8);
            typedef struct AtomU8   { U8   volatile  inner; } AtomU8;
ATOM__ALIGN(typedef struct AtomU16  { U16  volatile  inner; } AtomU16, 2);
ATOM__ALIGN(typedef struct AtomU32  { U32  volatile  inner; } AtomU32, 4);
ATOM__ALIGN(typedef struct AtomU64  { U64  volatile  inner; } AtomU64, 8);
ATOM__ALIGN(typedef struct AtomPtr  { void volatile *inner; } AtomPtr, CF_PTR_SIZE);
// clang-format on

#    if CF_COMPILER_MSVC
typedef AtomI32 AtomBool;
#    else
// clang-format off
            typedef struct AtomBool { bool volatile  inner; } AtomBool;
// clang-format on
#    endif

#    if CF_PTR_SIZE == 4
static_assert(sizeof(Isize) == 4 && sizeof(Usize) == 4, "Invalid size types");

typedef AtomI32 AtomIsize;
typedef AtomU32 AtomUsize;

#    elif CF_PTR_SIZE == 8
static_assert(sizeof(Isize) == 8 && sizeof(Usize) == 8, "Invalid size types");

typedef AtomI64 AtomIsize;
typedef AtomU64 AtomUsize;

#    else
#        error "Unsupported pointer size"
#    endif
#endif
