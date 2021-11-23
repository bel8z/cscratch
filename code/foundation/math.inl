#pragma once

/// Foundation math utilities
/// Lots of macros and inline functions - definetely NOT an API header

#include "core.h"
#include "error.h"

#include <immintrin.h>
#include <math.h>

//-----------------//
//   Miscellanea   //
//-----------------//

#define mAbs(X) _Generic((X), default : abs, I64 : llabs, F64 : fabs, F32 : fabsf)(X)
#define mSignBit(x) signbit(x)
#define mSign(x) (1 | mSignBit(x))
#define mCopySign(mag, sign) \
    _Generic(((mag), (sign)), default : copysign, F32 : copysignf)(mag, sign)

#define mCeil(X) _Generic((X), default : ceil, F32 : ceilf)(X)
#define mFloor(X) _Generic((X), default : floor, F32 : floorf)(X)
#define mRound(X) _Generic((X), default : round, F32 : roundf)(X)

//----------//
//   Trig   //
//----------//

#define M_PI64 3.1415926535897932384626433832795028841971693993751058209749445923078164062
#define M_PI32 3.1415926535897932384626433832795028841971693993751058209749445923078164062f

#define mCos(X) _Generic((X), default : cos, F32 : cosf)(X)
#define mSin(X) _Generic((X), default : sin, F32 : sinf)(X)
#define mTan(X) _Generic((X), default : tan, F32 : tanf)(X)

#define mAcos(X) _Generic((X), default : acos, F32 : acosf)(X)
#define mAsin(X) _Generic((X), default : asin, F32 : asinf)(X)
#define mAtan(X) _Generic((X), default : atan, F32 : atanf)(X)
#define mAtan2(X, Y) _Generic(((X), (Y)), default : atan2, F32 : atan2f)(X, Y)

#define mCosH(X) _Generic((X), default : cosh, F32 : coshf)(X)
#define mSinH(X) _Generic((X), default : sinh, F32 : sinhf)(X)
#define mTanH(X) _Generic((X), default : tanh, F32 : tanhf)(X)

#define mDegrees(X) _Generic((X), default : mDegrees64, F32 : mDegrees32)(X)
#define mRadians(X) _Generic((X), default : mRadians64, F32 : mRadians32)(X)

static inline F32
mDegrees32(F32 radians)
{
    return (radians * 180.0f) / M_PI32;
}

static inline F64
mDegrees64(F64 radians)
{
    return (radians * 180.0) / M_PI64;
}

static inline F32
mRadians32(F32 degrees)
{
    return (degrees * M_PI32) / 180.0f;
}

static inline F64
mRadians64(F64 degrees)
{
    return (degrees * M_PI64) / 180.0;
}

//-------------------//
//  Powers & roots   //
//-------------------//

#define mSqrt(X) _Generic((X), default : sqrt, F32 : sqrtf, I32 : mISqrt32, I64 : mISqrt64)(X)
#define mRsqrt(X) _Generic((X), default : (1 / mSqrt(X)), F32 : mRsqrt32(X))
#define mPow(base, xp) _Generic((base, xp), default : pow, F32 : powf)(base, xp)
#define mSquare(x) ((x) * (x))
#define mCube(x) ((x) * (x) * (x))
#define mExp(base, xp) _Generic((base, xp), default : exp, F32 : expf)(base, xp)
#define mLog(x) _Generic((x), default : log, F32 : logf)(x)

static inline F32
mRsqrt32(F32 x)
{
    return _mm_cvtss_f32(_mm_rsqrt_ss(_mm_set_ss(x)));
}

#define M__ISQRT(Size)                            \
    static inline I##Size mISqrt##Size(I##Size x) \
    {                                             \
        I##Size q = 1;                            \
        I##Size r = 0;                            \
                                                  \
        while (q <= x) q <<= 2;                   \
                                                  \
        while (q > 1)                             \
        {                                         \
            q >>= 2;                              \
                                                  \
            I##Size t = x - r - q;                \
                                                  \
            r >>= 1;                              \
                                                  \
            if (t >= 0)                           \
            {                                     \
                x = t;                            \
                r += q;                           \
            }                                     \
        }                                         \
                                                  \
        return r;                                 \
    }

M__ISQRT(32)
M__ISQRT(64)

#undef M__ISQRT

//-----------------//
//  Float modulo   //
//-----------------//

#define mFmod(X, Y) _Generic((X, Y), default : fmod, F32 : fmodf)(X, Y)

//------------------------------//
//  Integer division & modulo   //
//------------------------------//

// clang-format off

#define mDivEuclid(a, b)           \
    _Generic((a, b),                \
             I8  : m_I8DivEuclid,  \
             I16 : m_I16DivEuclid, \
             I32 : m_I32DivEuclid, \
             I64 : m_I64DivEuclid)(a, b)

#define mModEuclid(a, b)           \
    _Generic((a, b),                \
             I8  : m_I8ModEuclid,  \
             I16 : m_I16ModEuclid, \
             I32 : m_I32ModEuclid, \
             I64 : m_I64ModEuclid)(a, b)

#define mDivModEuclid(a, b, c)        \
    _Generic((a, b),                   \
             I8  : m_I8DivModEuclid,  \
             I16 : m_I16DivModEuclid, \
             I32 : m_I32DivModEuclid, \
             I64 : m_I64DivModEuclid)(a, b, c)

// clang-format on

#define M__EUCLID_OPS(Type)                                                  \
    static inline Type m_##Type##DivEuclid(Type lhs, Type rhs)               \
    {                                                                        \
        Type quot = lhs / rhs;                                               \
        Type rem = lhs % rhs;                                                \
        return (rem < 0) ? (rhs > 0 ? quot - 1 : quot + 1) : quot;           \
    }                                                                        \
                                                                             \
    static inline Type m_##Type##ModEuclid(Type lhs, Type rhs)               \
    {                                                                        \
        Type rem = lhs % rhs;                                                \
        return (rem < 0) ? (rhs < 0 ? rem - rhs : rem + rhs) : rem;          \
    }                                                                        \
                                                                             \
    static inline Type m_##Type##DivModEuclid(Type lhs, Type rhs, Type *mod) \
    {                                                                        \
        Type quot = lhs / rhs;                                               \
        Type rem = lhs % rhs;                                                \
                                                                             \
        if (rem < 0)                                                         \
        {                                                                    \
            *mod = (rhs < 0 ? rem - rhs : rem + rhs);                        \
            return (rhs > 0 ? quot - 1 : quot + 1);                          \
        }                                                                    \
                                                                             \
        *mod = rem;                                                          \
        return quot;                                                         \
    }

M__EUCLID_OPS(I8)
M__EUCLID_OPS(I16)
M__EUCLID_OPS(I32)
M__EUCLID_OPS(I64)

#undef M__EUCLID_OPS

// clang-format off

#define mMulDiv(a, b, c)        \
    _Generic((a, b, c),          \
             U8  : m_U8MulDiv,  \
             U16 : m_U16MulDiv, \
             U32 : m_U32MulDiv, \
             U64 : m_U64MulDiv, \
             I8  : m_I8MulDiv,  \
             I16 : m_I16MulDiv, \
             I32 : m_I32MulDiv, \
             I64 : m_I64MulDiv)(a, b, c)

// clang-format on

// Taken from the Rust code base:
// https://github.com/rust-lang/rust/blob/3809bbf47c8557bd149b3e52ceb47434ca8378d5/src/libstd/sys_common/mod.rs#L124
// Computes (value*numer)/denom without overflow, as long as both (numer*denom) and the overall
// result fit into i64 (which is the case for our time conversions).
#define M__MULDIV(Type)                                                        \
    static inline Type m_##Type##MulDiv(Type value, Type numer, Type denom)    \
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

M__MULDIV(U8)
M__MULDIV(U16)
M__MULDIV(U32)
M__MULDIV(U64)

M__MULDIV(I8)
M__MULDIV(I16)
M__MULDIV(I32)
M__MULDIV(I64)

#undef M__MULDIV

//---------//
//  Lerp   //
//---------//

#define mLerp(x, y, t) _Generic((x, y, t), default : mLerp64, F32 : mLerp32)(x, y, t)

static inline F32
mLerp32(F32 x, F32 y, F32 t)
{
    return x * (1 - t) + y * t;
}

static inline F64
mLerp64(F64 x, F64 y, F64 t)
{
    return x * (1 - t) + y * t;
}

//--------//
//  GCD   //
//--------//

// clang-format off

#define mGcd(a, b)                \
    _Generic((a, b),              \
             U8  : mGcdU8,        \
             U16 : mGcdU16,       \
             U32 : mGcdU32,       \
             U64 : mGcdU64)(a, b)

// clang-format on

#define M__GCD(Type)                                                             \
    static inline Type mGcd##Type(Type a, Type b)                                \
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

M__GCD(U8)
M__GCD(U16)
M__GCD(U32)
M__GCD(U64)

#undef M__GCD

//-------------------------------------//
//   N-dimensional vector operations   //
//-------------------------------------//

// NOTE (Matteo): Sorry for the macro abuse, but they are quite handy to generate all the functions
// for the most common arithmetic types. Also generic macros are convenient for reducing the typing
// effort (USER side obviously).

// clang-format off

/// Add two vectors of arbitrary length
#define vecAddN(a, b, length, out) \
    _Generic((a),                  \
             F32* : vec_F32AddN,   \
             F64* : vec_F64AddN,   \
             I32* : vec_I32AddN,   \
             I64* : vec_I64AddN)(a, b, length, out)

/// Subtract two vectors of arbitrary length
#define vecSubN(a, b, length, out) \
    _Generic((a),                  \
             F32* : vec_F32SubN,   \
             F64* : vec_F64SubN,   \
             I32* : vec_I32SubN,   \
             I64* : vec_I64SubN)(a, b, length, out)

/// Multiply a vector of arbitrary length times a scalar value
#define vecMulN(a, b, length, out) \
    _Generic((a),                  \
             F32* : vec_F32MulN,   \
             F64* : vec_F64MulN,   \
             I32* : vec_I32MulN,   \
             I64* : vec_I64MulN)(a, b, length, out)

/// Divide a vector of arbitrary length times a scalar value
#define vecDivN(a, b, length, out) \
    _Generic((a),                  \
             F32* : vec_F32DivN,   \
             F64* : vec_F64DivN,   \
             I32* : vec_I32DivN,   \
             I64* : vec_I64DivN)(a, b, length, out)

/// Dot product of two vectors of arbitrary length
#define vecDotN(a, b, length)    \
    _Generic((a),                \
             F32* : vec_F32DotN, \
             F64* : vec_F64DotN, \
             I32* : vec_I32DotN, \
             I64* : vec_I64DotN)(a, b, length)

/// Squared distance between two vectors of arbitrary length
#define vecDistanceSquaredN(a, b, length)    \
    _Generic((a),                            \
             F32* : vec_F32DistanceSquaredN, \
             F64* : vec_F64DistanceSquaredN, \
             I32* : vec_I32DistanceSquaredN, \
             I64* : vec_I64DistanceSquaredN)(a, b, length)

/// Distance between two vectors of arbitrary length
#define vecDistanceN(v, length) mSqrt(vecDistanceSquaredN(v, length))

/// Linear interpolation of two vectors of arbitrary length
#define vecLerpN(a, b, length, t, out) \
    _Generic((a),                      \
             F32* : vec_F32LerpN,      \
             F64* : vec_F64LerpN,      \
             I32* : vec_I32LerpN,      \
             I64* : vec_I64LerpN)(a, b, length, t, out)

/// Negate a vector of arbitrary length
#define vecNegateN(v, length, out)  \
    _Generic((v),                   \
             F32* : vec_F32NegateN, \
             F64* : vec_F64NegateN, \
             I32* : vec_I32NegateN, \
             I64* : vec_I64NegateN)(v, length, out)

// clang-format on

/// Squared norm of a vector of arbitrary length
#define vecNormSquaredN(v, length) vecDotN(v, v, length)

/// Norm of a vector of arbitrary length
#define vecNormN(v, length) mSqrt(vecNormSquaredN(v, length))

#define VEC__N_OPS(Scalar)                                                                      \
    static inline void vec_##Scalar##AddN(Scalar const *a, Scalar const *b, Usize length,       \
                                          Scalar *out)                                          \
    {                                                                                           \
        for (Usize n = 0; n < length; ++n) out[n] = a[n] + b[n];                                \
    }                                                                                           \
                                                                                                \
    static inline void vec_##Scalar##SubN(Scalar const *a, Scalar const *b, Usize length,       \
                                          Scalar *out)                                          \
    {                                                                                           \
        for (Usize n = 0; n < length; ++n) out[n] = a[n] - b[n];                                \
    }                                                                                           \
                                                                                                \
    static inline void vec_##Scalar##MulN(Scalar const *a, Scalar b, Usize length, Scalar *out) \
    {                                                                                           \
        for (Usize n = 0; n < length; ++n) out[n] = a[n] * b;                                   \
    }                                                                                           \
                                                                                                \
    static inline void vec_##Scalar##DivN(Scalar const *a, Scalar b, Usize length, Scalar *out) \
    {                                                                                           \
        for (Usize n = 0; n < length; ++n) out[n] = a[n] / b;                                   \
    }                                                                                           \
                                                                                                \
    static inline Scalar vec_##Scalar##DotN(Scalar const *a, Scalar const *b, Usize length)     \
    {                                                                                           \
        Scalar out = 0;                                                                         \
        for (Usize n = 0; n < length; ++n) out += a[n] * b[n];                                  \
        return out;                                                                             \
    }                                                                                           \
                                                                                                \
    static inline Scalar vec_##Scalar##DistanceSquaredN(Scalar const *a, Scalar const *b,       \
                                                        Usize length)                           \
    {                                                                                           \
                                                                                                \
        Scalar out = 0;                                                                         \
        for (Usize n = 0; n < length; ++n)                                                      \
        {                                                                                       \
            Scalar diff = a[n] - b[n];                                                          \
            out += diff * diff;                                                                 \
        }                                                                                       \
        return out;                                                                             \
    }                                                                                           \
                                                                                                \
    static inline void vec_##Scalar##LerpN(Scalar const *a, Scalar const *b, Usize length,      \
                                           Scalar t, Scalar *out)                               \
    {                                                                                           \
        Scalar t1 = (Scalar)1 - t;                                                              \
        for (Usize n = 0; n < length; ++n) out[n] = t1 * a[n] + t * b[n];                       \
    }                                                                                           \
                                                                                                \
    static inline void vec_##Scalar##NegateN(Scalar const *a, Usize length, Scalar *out)        \
    {                                                                                           \
        for (Usize n = 0; n < length; ++n) out[n] = -a[n];                                      \
    }

VEC__N_OPS(F32)
VEC__N_OPS(F64)
VEC__N_OPS(I32)
VEC__N_OPS(I64)
#undef VEC__N_OPS

//------------------------------------//
//   Type-generic vector operations   //
//------------------------------------//

#define VEC3_0 ((Vec3){0})
#define VEC3_X ((Vec3){.x = 1.0f, .y = 0.0f, .z = 0.0f})
#define VEC3_Y ((Vec3){.x = 0.0f, .y = 1.0f, .z = 0.0f})
#define VEC3_Z ((Vec3){.x = 0.0f, .y = 0.0f, .z = 1.0f})

// clang-format off

/// Add two vectors
#define vecAdd(a, b)                                          \
    _Generic(((a), (b)),                                      \
        Vec2  : vecAdd2,  Vec3  : vecAdd3,  Vec4  : vecAdd4,  \
        DVec2 : vecAdd2D, DVec3 : vecAdd3D, DVec4 : vecAdd4D, \
        IVec2 : vecAdd2I, IVec3 : vecAdd3I, IVec4 : vecAdd4I)(a, b)

/// Subtract two vectors
#define vecSub(a, b)                                          \
    _Generic(((a), (b)),                                      \
        Vec2  : vecSub2,  Vec3  : vecSub3,  Vec4  : vecSub4,  \
        DVec2 : vecSub2D, DVec3 : vecSub3D, DVec4 : vecSub4D, \
        IVec2 : vecSub2I, IVec3 : vecSub3I, IVec4 : vecSub4I)(a, b)

/// Multiply a vector by a scalar
#define vecMul(a, b)                                          \
    _Generic((a),                                             \
        Vec2  : vecMul2,  Vec3  : vecMul3,  Vec4  : vecMul4,  \
        DVec2 : vecMul2D, DVec3 : vecMul3D, DVec4 : vecMul4D, \
        IVec2 : vecMul2I, IVec3 : vecMul3I, IVec4 : vecMul4I)(a, b)

/// Divide a vector by a scalar
#define vecDiv(a, b)                                          \
    _Generic((a),                                             \
        Vec2  : vecDiv2,  Vec3  : vecDiv3,  Vec4  : vecDiv4,  \
        DVec2 : vecDiv2D, DVec3 : vecDiv3D, DVec4 : vecDiv4D, \
        IVec2 : vecDiv2I, IVec3 : vecDiv3I, IVec4 : vecDiv4I)(a, b)

/// Negate a vector
#define vecNegate(a)                                                   \
    _Generic((a),                                                      \
        Vec2  : vecNegate2,  Vec3  : vecNegate3,  Vec4  : vecNegate4,  \
        DVec2 : vecNegate2D, DVec3 : vecNegate3D, DVec4 : vecNegate4D, \
        IVec2 : vecNegate2I, IVec3 : vecNegate3I, IVec4 : vecNegate4I)(a)

/// Linear interpolation of two vectors
#define vecLerp(a, b, t)                                         \
    _Generic(((a), (b)),                                         \
        Vec2  : vecLerp2,  Vec3  : vecLerp3,  Vec4  : vecLerp4,  \
        DVec2 : vecLerp2D, DVec3 : vecLerp3D, DVec4 : vecLerp4D, \
        IVec2 : vecLerp2I, IVec3 : vecLerp3I, IVec4 : vecLerp4I)(a, b, t)

// clang-format on

/// Dot (scalar) product of two vectors
#define vecDot(a, b) vecDotN((a).elem, (b).elem, CF_ARRAY_SIZE((a).elem))

/// Squared distance between two vectors
#define vecDistanceSquared(a, b) vecDistanceSquaredN((a).elem, (b).elem, CF_ARRAY_SIZE((a).elem))

/// Distance between two vectors
#define vecDistance(a, b) mSqrt(vecDistanceSquared(a, b))

/// Squared norm of a vector
#define vecNormSquared(v) vecDot(v, v)

/// Norm of a vector
#define vecNorm(v) mSqrt(vecNormSquared(v))

/// Compute the normalized vector
#define vecNormalize(v) vecDiv(v, vecNorm(v))

/// The "perp dot product" a^_|_·b for a and b vectors in the plane is a modification of the
/// two-dimensional dot product in which a is replaced by the perpendicular vector rotated 90
/// degrees to the left defined by Hill (1994). It satisfies the identities
#define vecPerpDot(a, b) _Generic(((a), (b)), Vec2 : vecPerpDot2, DVec2 : vecPerpDot2D)(a, b)

/// Cross product of two 3D vectors
#define vecCross(a, b) _Generic(((a), (b)), Vec3 : vecCross3, DVec3 : vecCross3D)(a, b)

//-------------------------------------//
//   Type-specific vector operations   //
//-------------------------------------//

static inline F32
vecPerpDot2(Vec2 a, Vec2 b)
{
    return a.x * b.y - a.y * b.x;
}

static inline F64
vecPerpDot2D(DVec2 a, DVec2 b)
{
    return a.x * b.y - a.y * b.x;
}

static inline Vec3
vecCross3(Vec3 a, Vec3 b)
{
    return (Vec3){.x = a.y * b.z - a.z * b.y, //
                  .y = a.z * b.x - a.x * b.z,
                  .z = a.x * b.y - a.y * b.x};
}

static inline DVec3
vecCross3D(DVec3 a, DVec3 b)
{
    return (DVec3){.x = a.y * b.z - a.z * b.y, //
                   .y = a.z * b.x - a.x * b.z,
                   .z = a.x * b.y - a.y * b.x};
}

#define VEC__OPS(Scalar, N, tag)                                                      \
    static inline tag##Vec##N vecAdd##N##tag(tag##Vec##N a, tag##Vec##N b)            \
    {                                                                                 \
        tag##Vec##N out = {0};                                                        \
        vecAddN(a.elem, b.elem, N, out.elem);                                         \
        return out;                                                                   \
    }                                                                                 \
                                                                                      \
    static inline tag##Vec##N vecSub##N##tag(tag##Vec##N a, tag##Vec##N b)            \
    {                                                                                 \
        tag##Vec##N out = {0};                                                        \
        vecSubN(a.elem, b.elem, N, out.elem);                                         \
        return out;                                                                   \
    }                                                                                 \
                                                                                      \
    static inline tag##Vec##N vecMul##N##tag(tag##Vec##N a, Scalar b)                 \
    {                                                                                 \
        tag##Vec##N out = {0};                                                        \
        vecMulN(a.elem, b, N, out.elem);                                              \
        return out;                                                                   \
    }                                                                                 \
                                                                                      \
    static inline tag##Vec##N vecDiv##N##tag(tag##Vec##N a, Scalar b)                 \
    {                                                                                 \
        tag##Vec##N out = {0};                                                        \
        vecDivN(a.elem, b, N, out.elem);                                              \
        return out;                                                                   \
    }                                                                                 \
                                                                                      \
    static inline tag##Vec##N vecLerp##N##tag(tag##Vec##N a, tag##Vec##N b, Scalar t) \
    {                                                                                 \
        tag##Vec##N out = {0};                                                        \
        vecLerpN(a.elem, b.elem, N, t, out.elem);                                     \
        return out;                                                                   \
    }                                                                                 \
                                                                                      \
    static inline tag##Vec##N vecNegate##N##tag(tag##Vec##N a)                        \
    {                                                                                 \
        tag##Vec##N out = {0};                                                        \
        vecNegateN(a.elem, N, out.elem);                                              \
        return out;                                                                   \
    }

VEC__OPS(F32, 2, )
VEC__OPS(F32, 3, )
VEC__OPS(F32, 4, )

VEC__OPS(F64, 2, D)
VEC__OPS(F64, 3, D)
VEC__OPS(F64, 4, D)

VEC__OPS(I32, 2, I)
VEC__OPS(I32, 3, I)
VEC__OPS(I32, 4, I)

#undef VEC__OPS

//-----------------------//
//   Matrix operations   //
//-----------------------//

// NOTE (Matteo): Angles are always in radians!

//--- Common forms ---//

static inline Mat4
matDiagonal(F32 value)
{
    Mat4 mat = {0};
    mat.elem[0][0] = value;
    mat.elem[1][1] = value;
    mat.elem[2][2] = value;
    mat.elem[3][3] = value;
    return mat;
}

static inline Mat4
matIdentity(void)
{
    return matDiagonal(1);
}

static inline Mat4
matTranspose(Mat4 in)
{
    Mat4 out;

    out.elem[0][0] = in.elem[0][0];
    out.elem[1][0] = in.elem[0][1];
    out.elem[2][0] = in.elem[0][2];
    out.elem[3][0] = in.elem[0][3];

    out.elem[0][1] = in.elem[1][0];
    out.elem[1][1] = in.elem[1][1];
    out.elem[2][1] = in.elem[1][2];
    out.elem[3][1] = in.elem[1][3];

    out.elem[0][2] = in.elem[2][0];
    out.elem[1][2] = in.elem[2][1];
    out.elem[2][2] = in.elem[2][2];
    out.elem[3][2] = in.elem[2][3];

    out.elem[0][3] = in.elem[3][0];
    out.elem[1][3] = in.elem[3][1];
    out.elem[2][3] = in.elem[3][2];
    out.elem[3][3] = in.elem[3][3];

    return out;
}

//--- Transformations ---//

static inline Mat4
matScale(F32 value)
{
    Mat4 mat = {0};
    mat.elem[0][0] = value;
    mat.elem[1][1] = value;
    mat.elem[2][2] = value;
    mat.elem[3][3] = 1;
    return mat;
}

static inline Mat4
matTranslation(F32 x, F32 y, F32 z)
{
    Mat4 mat = {0};
    mat.elem[3][0] = x;
    mat.elem[3][1] = y;
    mat.elem[3][2] = z;
    return mat;
}

static inline Mat4
matTranslationVec(Vec3 t)
{
    return (Mat4){.cols[3] = {.xyz = t}};
}

static inline Mat4
matRotation(Vec3 axis, F32 radians)
{
    Mat4 mat = {0};

    axis = vecNormalize(axis);

    F32 sintheta = mSin(radians);
    F32 costheta = mCos(radians);
    F32 cosvalue = 1.0f - costheta;

    mat.elem[0][0] = (axis.x * axis.x * cosvalue) + costheta;
    mat.elem[0][1] = (axis.x * axis.y * cosvalue) + (axis.z * sintheta);
    mat.elem[0][2] = (axis.x * axis.z * cosvalue) - (axis.y * sintheta);

    mat.elem[1][0] = (axis.y * axis.x * cosvalue) - (axis.z * sintheta);
    mat.elem[1][1] = (axis.y * axis.y * cosvalue) + costheta;
    mat.elem[1][2] = (axis.y * axis.z * cosvalue) + (axis.x * sintheta);

    mat.elem[2][0] = (axis.z * axis.x * cosvalue) + (axis.y * sintheta);
    mat.elem[2][1] = (axis.z * axis.y * cosvalue) - (axis.x * sintheta);
    mat.elem[2][2] = (axis.z * axis.z * cosvalue) + costheta;

    mat.elem[3][3] = 1.0f;

    return mat;
}

static inline Mat4
matLookAtRh(Vec3 eye, Vec3 center, Vec3 up)
{
    Mat4 mat = {0};

    // See https://www.khronos.org/registry/OpenGL-Refpages/gl2.1/xhtml/gluLookAt.xml

    Vec3 s = vecSub(center, eye);
    Vec3 zaxis = vecNormalize(s);

    Vec3 c = vecCross(zaxis, up);
    Vec3 xaxis = vecNormalize(c);

    Vec3 yaxis = vecCross(xaxis, zaxis);

    // Set the rows to the new axes (since this is an inverse transform)
    mat.elem[0][0] = xaxis.x;
    mat.elem[1][0] = xaxis.y;
    mat.elem[2][0] = xaxis.z;

    mat.elem[0][1] = yaxis.x;
    mat.elem[1][1] = yaxis.y;
    mat.elem[2][1] = yaxis.z;

    mat.elem[0][2] = -zaxis.x;
    mat.elem[1][2] = -zaxis.y;
    mat.elem[2][2] = -zaxis.z;

    // Set translation
    mat.elem[3][0] = -vecDot(xaxis, eye);
    mat.elem[3][1] = -vecDot(yaxis, eye);
    mat.elem[3][2] = vecDot(zaxis, eye);

    mat.elem[3][3] = 1.0f;

    return mat;
}

static inline Mat4
matLookAtLh(Vec3 eye, Vec3 center, Vec3 up)
{
    Mat4 mat = {0};

    Vec3 s = vecSub(center, eye);
    Vec3 zaxis = vecNormalize(s);

    Vec3 c = vecCross(up, zaxis);
    Vec3 xaxis = vecNormalize(c);

    Vec3 yaxis = vecCross(zaxis, xaxis);

    // Set the rows to the new axes (since this is an inverse transform)
    mat.elem[0][0] = xaxis.x;
    mat.elem[1][0] = xaxis.y;
    mat.elem[2][0] = xaxis.z;

    mat.elem[0][1] = yaxis.x;
    mat.elem[1][1] = yaxis.y;
    mat.elem[2][1] = yaxis.z;

    mat.elem[0][2] = zaxis.x;
    mat.elem[1][2] = zaxis.y;
    mat.elem[2][2] = zaxis.z;

    // Set translation
    mat.elem[3][0] = -vecDot(xaxis, eye);
    mat.elem[3][1] = -vecDot(yaxis, eye);
    mat.elem[3][2] = -vecDot(zaxis, eye);

    mat.elem[3][3] = 1.0f;

    return mat;
}

static inline Mat4
matPerspective(F32 fovy, F32 aspect, F32 z_near, F32 z_far, I32 y_dir)
{
    Mat4 mat = {0};

    // See:
    // https://www.khronos.org/registry/OpenGL-Refpages/gl2.1/xhtml/gluPerspective.xml
    // http://www.songho.ca/opengl/gl_projectionmatrix.html

    F32 f = 1 / mTan(fovy / 2);

    mat.elem[0][0] = f * aspect;

    // NOTE (Matteo): Handle coordinate systems with y-up and y-down
    CF_ASSERT(y_dir == -1 || y_dir == 1, "Invalid Y axis direction");
    mat.elem[1][1] = f * (F32)y_dir;

    mat.elem[2][2] = (z_near + z_far) / (z_near - z_far);
    mat.elem[2][3] = -1.0f;

    mat.elem[3][2] = (2.0f * z_near * z_far) / (z_near - z_far);

    return mat;
}

//--- Algebra ---//

// clang-format off

#define matMul(left, right) \
    _Generic(((right)),     \
        Mat4 : matMulMat4,  \
        Vec2 : matMulVec2,  \
        Vec3 : matMulVec3,  \
        Vec4 : matMulVec4)(left, right)

// clang-format on

static inline Mat4
matMulMat4(Mat4 left, Mat4 right)
{
    // TODO (Matteo): Go wide with SIMD

    Mat4 mat = {0};

    for (U32 col = 0; col < 4; ++col)
    {
        for (U32 row = 0; row < 4; ++row)
        {
            F32 sum = 0;

            for (U32 cur = 0; cur < 4; ++cur)
            {
                sum += left.elem[cur][row] * right.elem[col][cur];
            }

            mat.elem[col][row] = sum;
        }
    }

    return mat;
}

static inline Vec4
matMulVec4(Mat4 mat, Vec4 vec)
{
    Vec4 res = {0};

    // TODO (Matteo): Go wide with SIMD
    for (Usize col = 0; col < 4; ++col)
    {
        res = vecAdd(res, vecMul(mat.cols[col], vec.elem[col]));
    }

    return res;
}

static inline Vec3
matMulVec3(Mat4 mat, Vec3 vec)
{
    return matMulVec4(mat, (Vec4){.xyz = vec}).xyz;
}

static inline Vec2
matMulVec2(Mat4 mat, Vec2 vec)
{
    return (Vec2){
        .x = mat.elem[0][0] * vec.x + mat.elem[1][0] * vec.y + mat.elem[3][0],
        .y = mat.elem[0][1] * vec.x + mat.elem[1][1] * vec.y + mat.elem[3][1],
    };
}
