#pragma once

#include "core.h"

// Atomic Types

#if CF_COMPILER_CLANG && __has_builtin(__c11_atomic_init)
#    define ATOM__CLANG_BUILTIN 0
#else
#    define ATOM__CLANG_BUILTIN 0
#endif

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
#    if CF_COMPILER_MSVC
#        define ATOM_ENSURE_ALIGN(decl, amt) __declspec(align(amt)) decl
#    else
#        define ATOM_ENSURE_ALIGN(decl, amt) decl __attribute__((aligned(amt)))
#    endif

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
