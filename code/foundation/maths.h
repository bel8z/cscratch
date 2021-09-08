#pragma once

#include "core.h"

#include <immintrin.h>
#include <math.h>

//-----------------//
//   Miscellanea   //
//-----------------//

#define cfAbs(X) _Generic((X), default : abs, I64 : llabs, F64 : fabs, F32 : fabsf)(X)
#define cfSignBit(x) signbit(x)
#define cfSign(x) (1 | cfSignBit(x))
#define cfCopySign(mag, sign) _Generic((mag, sign), default : copysign, F32 : copysignf)(mag, sign)

#define cfCeil(X) _Generic((X), default : ceil, F32 : ceilf)(X)
#define cfFloor(X) _Generic((X), default : floor, F32 : floorf)(X)
#define cfRound(X) _Generic((X), default : round, F32 : roundf)(X)

//----------//
//   Trig   //
//----------//

#define CF_PI64 3.1415926535897932384626433832795028841971693993751058209749445923078164062
#define CF_PI32 3.1415926535897932384626433832795028841971693993751058209749445923078164062f

#define cfCos(X) _Generic((X), default : cos, F32 : cosf)(X)
#define cfSin(X) _Generic((X), default : sin, F32 : sinf)(X)
#define cfTan(X) _Generic((X), default : tan, F32 : tanf)(X)

#define cfAcos(X) _Generic((X), default : acos, F32 : acosf)(X)
#define cfAsin(X) _Generic((X), default : asin, F32 : asinf)(X)
#define cfAtan(X) _Generic((X), default : atan, F32 : atanf)(X)
#define cfAtan2(X, Y) _Generic((X, Y), default : atan2, F32 : atan2f)(X, Y)

#define cfCosH(X) _Generic((X), default : cosh, F32 : coshf)(X)
#define cfSinH(X) _Generic((X), default : sinh, F32 : sinhf)(X)
#define cfTanH(X) _Generic((X), default : tanh, F32 : tanhf)(X)

//-------------------//
//  Powers & roots   //
//-------------------//

#define cfSqrt(X) _Generic((X), default : sqrt, F32 : sqrtf, I32 : ISqrt32, I64 : ISqrt64)(X)
#define cfRsqrt(X) _Generic((X), default : (1 / cfSqrt(X)), F32 : cfRsqrt32(X))
#define cfPow(base, xp) _Generic((base, xp), default : pow, F32 : powf)(base, xp)
#define cfSquare(x) ((x) * (x))
#define cfCube(x) ((x) * (x) * (x))
#define cfExp(base, xp) _Generic((base, xp), default : exp, F32 : expf)(base, xp)
#define cfLog(x) _Generic((x), default : log, F32 : logf)(x)

static inline F32
cfRsqrt32(F32 x)
{
    return _mm_cvtss_f32(_mm_rsqrt_ss(_mm_set_ss(x)));
}

static inline I32
ISqrt32(I32 x)
{
    I32 q = 1;
    I32 r = 0;

    while (q <= x) q <<= 2;

    while (q > 1)
    {
        q >>= 2;

        I32 t = x - r - q;

        r >>= 1;

        if (t >= 0)
        {
            x = t;
            r += q;
        }
    }

    return r;
}

static inline I64
ISqrt64(I64 x)
{
    I64 q = 1;
    I64 r = 0;

    while (q <= x) q <<= 2;

    while (q > 1)
    {
        q >>= 2;

        I64 t = x - r - q;

        r >>= 1;

        if (t >= 0)
        {
            x = t;
            r += q;
        }
    }

    return r;
}

//-----------------//
//  Float modulo   //
//-----------------//

#define cfFmod(X, Y) _Generic((X, Y), default : fmod, F32 : fmodf)(X, Y)

//------------------------------//
//  Integer division & modulo   //
//------------------------------//

// clang-format off
#define cfDivEuclid(a, b)          \
    _Generic((a, b),               \
             I8  : cfDivEuclidI8,  \
             I16 : cfDivEuclidI16, \
             I32 : cfDivEuclidI32, \
             I64 : cfDivEuclidI64)(a, b)

#define cfModEuclid(a, b)          \
    _Generic((a, b),               \
             I8  : cfModEuclidI8,  \
             I16 : cfModEuclidI16, \
             I32 : cfModEuclidI32, \
             I64 : cfModEuclidI64)(a, b)

#define cfDivModEuclid(a, b, c)       \
    _Generic((a, b),                  \
             I8  : cfDivModEuclidI8,  \
             I16 : cfDivModEuclidI16, \
             I32 : cfDivModEuclidI32, \
             I64 : cfDivModEuclidI64)(a, b, c)

// clang-format on

#define CF__EUCLID_OPS(Type)                                               \
    static inline Type cfDivEuclid##Type(Type lhs, Type rhs)               \
    {                                                                      \
        Type quot = lhs / rhs;                                             \
        Type rem = lhs % rhs;                                              \
        return (rem < 0) ? (rhs > 0 ? quot - 1 : quot + 1) : quot;         \
    }                                                                      \
                                                                           \
    static inline Type cfModEuclid##Type(Type lhs, Type rhs)               \
    {                                                                      \
        Type rem = lhs % rhs;                                              \
        return (rem < 0) ? (rhs < 0 ? rem - rhs : rem + rhs) : rem;        \
    }                                                                      \
                                                                           \
    static inline Type cfDivModEuclid##Type(Type lhs, Type rhs, Type *mod) \
    {                                                                      \
        Type quot = lhs / rhs;                                             \
        Type rem = lhs % rhs;                                              \
                                                                           \
        if (rem < 0)                                                       \
        {                                                                  \
            *mod = (rhs < 0 ? rem - rhs : rem + rhs);                      \
            return (rhs > 0 ? quot - 1 : quot + 1);                        \
        }                                                                  \
                                                                           \
        *mod = rem;                                                        \
        return quot;                                                       \
    }

CF__EUCLID_OPS(I8)
CF__EUCLID_OPS(I16)
CF__EUCLID_OPS(I32)
CF__EUCLID_OPS(I64)

#undef CF__EUCLID_OPS

// clang-format off
#define cfMulDiv(a, b, c)       \
    _Generic((a, b, c),         \
             U8  : cfMulDivU8,  \
             U16 : cfMulDivU16, \
             U32 : cfMulDivU32, \
             U64 : cfMulDivU64, \
             I8  : cfMulDivI8,  \
             I16 : cfMulDivI16, \
             I32 : cfMulDivI32, \
             I64 : cfMulDivI64)(a, b, c)
// clang-format on

// Taken from the Rust code base:
// https://github.com/rust-lang/rust/blob/3809bbf47c8557bd149b3e52ceb47434ca8378d5/src/libstd/sys_common/mod.rs#L124
// Computes (value*numer)/denom without overflow, as long as both (numer*denom) and the overall
// result fit into i64 (which is the case for our time conversions).
#define CF__MULDIV(Type)                                                       \
    static inline Type cfMulDiv##Type(Type value, Type numer, Type denom)      \
    {                                                                          \
        CF_DEBUG_ASSERT(numer *denom < T_MAX(Type), "Operation can overflow"); \
                                                                               \
        Type q = value / denom;                                                \
        Type r = value % denom;                                                \
        /*                                                                     \
        Decompose value as (value/denom*denom + value%denom), substitute into  \
        (value*numer)/denom and simplify.                                      \
        r < denom, so (denom*numer) is the upper bound of (r*numer)            \
        */                                                                     \
        return q * numer + r * numer / denom;                                  \
    }

CF__MULDIV(U8)
CF__MULDIV(U16)
CF__MULDIV(U32)
CF__MULDIV(U64)
CF__MULDIV(I8)
CF__MULDIV(I16)
CF__MULDIV(I32)
CF__MULDIV(I64)

#undef CF__MULDIV

//---------//
//  Lerp   //
//---------//

#define cfLerp(x, y, t) _Generic((x, y, t), default : cfLerp64, F32 : cfLerp32)(x, y, t)

static inline F32
cfLerp32(F32 x, F32 y, F32 t)
{
    return x * (1 - t) + y * t;
}

static inline F64
cfLerp64(F64 x, F64 y, F64 t)
{
    return x * (1 - t) + y * t;
}

//--------//
//  GCD   //
//--------//

// clang-format off
#define cfGcd(a, b)                \
    _Generic((a, b),               \
             U8  : cfGcdU8,        \
             U16 : cfGcdU16,       \
             U32 : cfGcdU32,       \
             U64 : cfGcdU64)(a, b)
// clang-format on

#define CF__GCD(Type)                                                            \
    static inline Type cfGcd##Type(Type a, Type b)                               \
    {                                                                            \
        /* GCD(0, b) == b, */                                                    \
        /* GCD(a, 0) == a, */                                                    \
        /* GCD(0, 0) == 0  */                                                    \
                                                                                 \
        if (a == 0) return b;                                                    \
        if (b == 0) return a;                                                    \
                                                                                 \
        /* Find k, which is the greatest power of 2 that divides both a and b */ \
        Type k = 0;                                                              \
        for (; !((a | b) & 1); ++k)                                              \
        {                                                                        \
            a = a >> 1;                                                          \
            b = b >> 1;                                                          \
        }                                                                        \
                                                                                 \
        /* Divide a by 2 until it becames odd */                                 \
        while (!(a & 1)) a = a >> 1;                                             \
                                                                                 \
        do                                                                       \
        {                                                                        \
            /* Divide b by 2 until it becames odd */                             \
            while (!(b & 1)) b = b >> 1;                                         \
                                                                                 \
            /* Now a and b are both odd. Swap in order to have a <= b, */        \
            /* then set b = b - a (which is even). */                            \
            if (a > b)                                                           \
            {                                                                    \
                Type t = a;                                                      \
                a = b;                                                           \
                b = t;                                                           \
            }                                                                    \
                                                                                 \
            b = (b - a);                                                         \
        } while (b);                                                             \
                                                                                 \
        /* Restore the common factor of 2 */                                     \
        return (Type)(a << k);                                                   \
    }

CF__GCD(U8)
CF__GCD(U16)
CF__GCD(U32)
CF__GCD(U64)

#undef CF__GCD

//-------------------------------------//
//   N-dimensional vector operations   //
//-------------------------------------//

// NOTE (Matteo): Sorry for the macro abuse, but they are quite handy to generate all the functions
// for the most common arithmetic types. Also generic macros are convenient for reducing the typing
// effort (USER side obviously).

// clang-format off

/// Add two vectors of arbitrary length
#define vecAddN(a, b, length, out)     \
    _Generic((a),                      \
             F32* : vec_F32AddN, \
             F64* : vec_F64AddN, \
             I32* : vec_I32AddN, \
             I64* : vec_I64AddN)(a, b, length, out)

/// Subtract two vectors of arbitrary length
#define vecSubN(a, b, length, out)     \
    _Generic((a),                      \
             F32* : vec_F32SubN, \
             F64* : vec_F64SubN, \
             I32* : vec_I32SubN, \
             I64* : vec_I64SubN)(a, b, length, out)

/// Multiply a vector of arbitrary length times a scalar value
#define vecMulN(a, b, length, out)     \
    _Generic((a),                      \
             F32* : vec_F32MulN, \
             F64* : vec_F64MulN, \
             I32* : vec_I32MulN, \
             I64* : vec_I64MulN)(a, b, length, out)

/// Divide a vector of arbitrary length times a scalar value
#define vecDivN(a, b, length, out)          \
    _Generic((a),               \
             F32* : vec_F32DivN,  \
             F64* : vec_F64DivN, \
             I32* : vec_I32DivN, \
             I64* : vec_I64DivN)(a, b, length, out)

/// Dot product of two vectors of arbitrary length
#define vecDotN(a, b, length)          \
    _Generic((a),               \
             F32* : vec_F32DotN,  \
             F64* : vec_F64DotN, \
             I32* : vec_I32DotN, \
             I64* : vec_I64DotN)(a, b, length)

/// Squared distance between two vectors of arbitrary length
#define vecDistanceSquaredN(a, b, length)          \
    _Generic((a),               \
             F32* : vec_F32DistanceSquaredN,  \
             F64* : vec_F64DistanceSquaredN, \
             I32* : vec_I32DistanceSquaredN, \
             I64* : vec_I64DistanceSquaredN)(a, b, length)

/// Distance between two vectors of arbitrary length
#define vecDistanceN(a, b, length)          \
    _Generic((a),               \
             F32* : vec_F32DistanceN,  \
             F64* : vec_F64DistanceN, \
             I32* : vec_I32DistanceN, \
             I64* : vec_I64DistanceN)(a, b, length)

/// Linear interpolation of two vectors of arbitrary length
#define vecLerpN(a, b, length, t, out)          \
    _Generic((a),               \
             F32* : vec_F32LerpN,  \
             F64* : vec_F64LerpN, \
             I32* : vec_I32LerpN, \
             I64* : vec_I64LerpN)(a, b, length, t, out)

/// Negate a vector of arbitrary length
#define vecNegateN(v, length, out)          \
    _Generic((v),               \
             F32* : vec_F32NegateN,  \
             F64* : vec_F64NegateN, \
             I32* : vec_I32NegateN, \
             I64* : vec_I64NegateN)(v, length, out)

/// Squared norm of a vector of arbitrary length
#define vecNormSquaredN(v, length)             \
    _Generic((v),                              \
             F32* : vec_F32NormSquaredN, \
             F64* : vec_F64NormSquaredN, \
             I32* : vec_I32NormSquaredN, \
             I64* : vec_I64NormSquaredN)(v, length)

/// Norm of a vector of arbitrary length
#define vecNormN(v, length)             \
    _Generic((v),                       \
             F32* : vec_F32NormN, \
             F64* : vec_F64NormN, \
             I32* : vec_I32NormN, \
             I64* : vec_I64NormN)(v, length)

// clang-format on

#define VEC__N_OPS(Scalar)                                                                       \
    static inline void vec_##Scalar##AddN(Scalar const *a, Scalar const *b, Usize length,        \
                                          Scalar *out)                                           \
    {                                                                                            \
        for (Usize n = 0; n < length; ++n) out[n] = a[n] + b[n];                                 \
    }                                                                                            \
                                                                                                 \
    static inline void vec_##Scalar##SubN(Scalar const *a, Scalar const *b, Usize length,        \
                                          Scalar *out)                                           \
    {                                                                                            \
        for (Usize n = 0; n < length; ++n) out[n] = a[n] - b[n];                                 \
    }                                                                                            \
                                                                                                 \
    static inline void vec_##Scalar##MulN(Scalar const *a, Scalar b, Usize length, Scalar *out)  \
    {                                                                                            \
        for (Usize n = 0; n < length; ++n) out[n] = a[n] * b;                                    \
    }                                                                                            \
                                                                                                 \
    static inline void vec_##Scalar##DivN(Scalar const *a, Scalar b, Usize length, Scalar *out)  \
    {                                                                                            \
        for (Usize n = 0; n < length; ++n) out[n] = a[n] / b;                                    \
    }                                                                                            \
                                                                                                 \
    static inline Scalar vec_##Scalar##DotN(Scalar const *a, Scalar const *b, Usize length)      \
    {                                                                                            \
        Scalar out = 0;                                                                          \
        for (Usize n = 0; n < length; ++n) out += a[n] * b[n];                                   \
        return out;                                                                              \
    }                                                                                            \
                                                                                                 \
    static inline Scalar vec_##Scalar##DistanceSquaredN(Scalar const *a, Scalar const *b,        \
                                                        Usize length)                            \
    {                                                                                            \
                                                                                                 \
        Scalar out = 0;                                                                          \
        for (Usize n = 0; n < length; ++n)                                                       \
        {                                                                                        \
            Scalar diff = a[n] - b[n];                                                           \
            out += diff * diff;                                                                  \
        }                                                                                        \
        return out;                                                                              \
    }                                                                                            \
                                                                                                 \
    static inline Scalar vec_##Scalar##DistanceN(Scalar const *a, Scalar const *b, Usize length) \
    {                                                                                            \
        return cfSqrt(vec_##Scalar##DistanceSquaredN(a, b, length));                             \
    }                                                                                            \
                                                                                                 \
    static inline void vec_##Scalar##LerpN(Scalar const *a, Scalar const *b, Usize length,       \
                                           Scalar t, Scalar *out)                                \
    {                                                                                            \
        Scalar t1 = (Scalar)1 - t;                                                               \
        for (Usize n = 0; n < length; ++n) out[n] = t1 * a[n] + t * b[n];                        \
    }                                                                                            \
                                                                                                 \
    static inline void vec_##Scalar##NegateN(Scalar const *a, Usize length, Scalar *out)         \
    {                                                                                            \
        for (Usize n = 0; n < length; ++n) out[n] = -a[n];                                       \
    }                                                                                            \
                                                                                                 \
    static inline Scalar vec_##Scalar##NormSquaredN(Scalar const *a, Usize length)               \
    {                                                                                            \
        return vec_##Scalar##DotN(a, a, length);                                                 \
    }                                                                                            \
                                                                                                 \
    static inline Scalar vec_##Scalar##NormN(Scalar const *a, Usize length)                      \
    {                                                                                            \
        return cfSqrt(vec_##Scalar##NormSquaredN(a, length));                                    \
    }

VEC__N_OPS(F32)
VEC__N_OPS(F64)
VEC__N_OPS(I32)
VEC__N_OPS(I64)
#undef VEC__N_OPS

//------------------------------------//
//   Type-generic vector operations   //
//------------------------------------//

#define vecAdd(a, b) _Generic((a, b), Vec2 : vecAdd2, Vec3 : vecAdd3, Vec4 : vecAdd4)(a, b)
#define vecSub(a, b) _Generic((a, b), Vec2 : vecSub2, Vec3 : vecSub3, Vec4 : vecSub4)(a, b)
#define vecMul(a, b) _Generic((a), Vec2 : vecMul2, Vec3 : vecMul3, Vec4 : vecMul4)(a, b)
#define vecDiv(a, b) _Generic((a), Vec2 : vecDiv2, Vec3 : vecDiv3, Vec4 : vecDiv4)(a, b)

#define vecNegate(a) _Generic((a), Vec2 : vecNegate2, Vec3 : vecNegate3, Vec4 : vecNegate4)(a)

#define vecDot(a, b) _Generic((a, b), Vec2 : vecDot2, Vec3 : vecDot3, Vec4 : vecDot4)(a, b)

#define vecNormSquared(a) \
    _Generic((a), Vec2 : vecNormSquared2, Vec3 : vecNormSquared3, Vec4 : vecNormSquared4)(a)

#define vecNorm(a) _Generic((a), Vec2 : vecNorm2, Vec3 : vecNorm3, Vec4 : vecNorm4)(a)

#define vecNormalize(a) vecDiv(a, vecNorm(a))

#define vecDistanceSquared(a, b)         \
    _Generic((a, b), Vec2                \
             : vecDistanceSquared2, Vec3 \
             : vecDistanceSquared3, Vec4 \
             : vecDistanceSquared4)(a, b)

#define vecDistance(a, b) \
    _Generic((a, b), Vec2 : vecDistance2, Vec3 : vecDistance3, Vec4 : vecDistance4)(a, b)

#define vecLerp(a, b) _Generic((a), Vec2 : vecLerp2, Vec3 : vecLerp3, Vec4 : vecLerp4)(a, b)

#define vecDotPerp(a, b) _Generic((a, b), Vec2 : vecDotPerp2)(a, b)

#define vecCross(a, b) _Generic((a, b), Vec3 : vecCross3)(a, b)

//-------------------------------------//
//   Type-specific vector operations   //
//-------------------------------------//

static inline F32
vecDotPerp2(Vec2 a, Vec2 b)
{
    return a.x * b.y - a.y * b.x;
}

static inline Vec3
vecCross3(Vec3 a, Vec3 b)
{
    return (Vec3){.x = a.y * b.z - a.z + b.y, //
                  .y = a.z * b.x - a.x * b.z,
                  .z = a.x * b.y - a.y * b.x};
}

#define VEC__OPS(N)                                                                        \
    static inline Vec##N vecAdd##N(Vec##N a, Vec##N b)                                     \
    {                                                                                      \
        Vec##N out = {0};                                                                  \
        vecAddN(a.elem, b.elem, N, out.elem);                                              \
        return out;                                                                        \
    }                                                                                      \
                                                                                           \
    static inline Vec##N vecSub##N(Vec##N a, Vec##N b)                                     \
    {                                                                                      \
        Vec##N out = {0};                                                                  \
        vecSubN(a.elem, b.elem, N, out.elem);                                              \
        return out;                                                                        \
    }                                                                                      \
                                                                                           \
    static inline Vec##N vecMul##N(Vec##N a, F32 b)                                        \
    {                                                                                      \
        Vec##N out = {0};                                                                  \
        vecMulN(a.elem, b, N, out.elem);                                                   \
        return out;                                                                        \
    }                                                                                      \
                                                                                           \
    static inline Vec##N vecDiv##N(Vec##N a, F32 b)                                        \
    {                                                                                      \
        Vec##N out = {0};                                                                  \
        vecDivN(a.elem, b, N, out.elem);                                                   \
        return out;                                                                        \
    }                                                                                      \
                                                                                           \
    static inline Vec##N vecNegate##N(Vec##N a)                                            \
    {                                                                                      \
        Vec##N out = {0};                                                                  \
        vecNegateN(a.elem, N, out.elem);                                                   \
        return out;                                                                        \
    }                                                                                      \
                                                                                           \
    static inline F32 vecDot##N(Vec##N a, Vec##N b) { return vecDotN(a.elem, b.elem, N); } \
                                                                                           \
    static inline F32 vecNormSquared##N(Vec##N a) { return vecNormSquaredN(a.elem, N); }   \
                                                                                           \
    static inline F32 vecNorm##N(Vec##N a) { return vecNormN(a.elem, N); }                 \
                                                                                           \
    static inline F32 vecDistanceSquared##N(Vec##N a, Vec##N b)                            \
    {                                                                                      \
        return vecDistanceSquaredN(a.elem, b.elem, N);                                     \
    }                                                                                      \
                                                                                           \
    static inline F32 vecDistance##N(Vec##N a, Vec##N b)                                   \
    {                                                                                      \
        return vecDistanceN(a.elem, b.elem, N);                                            \
    }                                                                                      \
                                                                                           \
    static inline Vec##N vecLerp##N(Vec##N a, Vec##N b, F32 t)                             \
    {                                                                                      \
        Vec##N out = {0};                                                                  \
        vecLerpN(a.elem, b.elem, N, t, out.elem);                                          \
        return out;                                                                        \
    }

VEC__OPS(2)
VEC__OPS(3)
VEC__OPS(4)
#undef VEC__OPS

// Mat 4
// TODO
