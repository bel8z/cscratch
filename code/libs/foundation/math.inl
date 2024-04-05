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

#define mAbs(X) _Generic((X), default: abs, I64: llabs, double: fabs, float: fabsf)(X)
#define mSignBit(x) signbit(x)
#define mSign(x) (1 | mSignBit(x))
#define mCopySign(mag, sign) \
    _Generic(((mag), (sign)), default: copysign, float: copysignf)(mag, sign)

#define mCeil(X) _Generic((X), default: ceil, float: ceilf)(X)
#define mFloor(X) _Generic((X), default: floor, float: floorf)(X)
#define mRound(X) _Generic((X), default: round, float: roundf)(X)

//----------//
//   Trig   //
//----------//

#define M_PI64 3.1415926535897932384626433832795028841971693993751058209749445923078164062
#define M_PI32 3.1415926535897932384626433832795028841971693993751058209749445923078164062f

#define mCos(X) _Generic((X), default: cos, float: cosf)(X)
#define mSin(X) _Generic((X), default: sin, float: sinf)(X)
#define mTan(X) _Generic((X), default: tan, float: tanf)(X)

#define mAcos(X) _Generic((X), default: acos, float: acosf)(X)
#define mAsin(X) _Generic((X), default: asin, float: asinf)(X)
#define mAtan(X) _Generic((X), default: atan, float: atanf)(X)
#define mAtan2(X, Y) _Generic(((X), (Y)), default: atan2, float: atan2f)(X, Y)

#define mCosH(X) _Generic((X), default: cosh, float: coshf)(X)
#define mSinH(X) _Generic((X), default: sinh, float: sinhf)(X)
#define mTanH(X) _Generic((X), default: tanh, float: tanhf)(X)

#define mDegrees(X) _Generic((X), default: mDegrees64, float: mDegrees32)(X)
#define mRadians(X) _Generic((X), default: mRadians64, float: mRadians32)(X)

static inline float
mDegrees32(float radians)
{
    return (radians * 180.0f) / M_PI32;
}

static inline double
mDegrees64(double radians)
{
    return (radians * 180.0) / M_PI64;
}

static inline float
mRadians32(float degrees)
{
    return (degrees * M_PI32) / 180.0f;
}

static inline double
mRadians64(double degrees)
{
    return (degrees * M_PI64) / 180.0;
}

//-------------------//
//  Powers & roots   //
//-------------------//

#define mSqrt(X) _Generic((X), default: sqrt, float: sqrtf, I32: mISqrt32, I64: mISqrt64)(X)
#define mRsqrt(X) _Generic((X), default: (1 / mSqrt(X)), float: mRsqrt32(X))
#define mCbrt(X) _Generic((X), default: cbrt, float: cbrtf)(X)
#define mPow(base, xp) _Generic((base, xp), default: pow, float: powf)(base, xp)
#define mSquare(x) ((x) * (x))
#define mCube(x) ((x) * (x) * (x))
#define mExp(xp) _Generic((xp), default: exp, float: expf)(xp)
#define mLog(x) _Generic((x), default: log, float: logf)(x)
#define mLog10(x) _Generic((x), default: log10, float: logf10)(x)
#define mLog2(x) _Generic((x), default: log2, float: logf2)(x)

static inline float
mRsqrt32(float x)
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

#define mFmod(X, Y) _Generic((X, Y), default: fmod, float: fmodf)(X, Y)

//------------------------------//
//  Integer division & modulo   //
//------------------------------//

// clang-format off

#define mDivEuclid(a, b)           \
    _Generic((a, b),                \
             I8  : m_I8DivEuclid,  \
             I16 : m_I16DivEuclid, \
             I32 : m_I32divEuclid, \
             I64 : m_I64divEuclid)(a, b)

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
             I32 : m_I32divModEuclid, \
             I64 : m_I64divModEuclid)(a, b, c)

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

#define mLerp(x, y, t) _Generic((x, y, t), default: mLerp64, float: mLerp32)(x, y, t)

static inline float
mLerp32(float x, float y, float t)
{
    return x * (1 - t) + y * t;
}

static inline double
mLerp64(double x, double y, double t)
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
             float*  : vec_AddNf,  \
             double* : vec_AddNd,  \
             I32*    : vec_AddNi,  \
             I64*    : vec_AddNl)(a, b, length, out)

/// Subtract two vectors of arbitrary length
#define vecSubN(a, b, length, out) \
    _Generic((a),                  \
             float*  : vec_SubNf,  \
             double* : vec_SubNd,  \
             I32*    : vec_SubNi,  \
             I64*    : vec_SubNl)(a, b, length, out)

/// Multiply a vector of arbitrary length times a scalar value
#define vecMulN(a, b, length, out) \
    _Generic((a),                  \
             float*  : vec_MulNf,  \
             double* : vec_MulNd,  \
             I32*    : vec_MulNi,  \
             I64*    : vec_MulNl)(a, b, length, out)

/// Divide a vector of arbitrary length times a scalar value
#define vecDivN(a, b, length, out) \
    _Generic((a),                  \
             float*  : vec_DivNf,  \
             double* : vec_DivNd,  \
             I32*    : vec_DivNi,  \
             I64*    : vec_DivNl)(a, b, length, out)

/// Dot product of two vectors of arbitrary length
#define vecDotN(a, b, length)     \
    _Generic((a),                 \
             float*  : vec_DotNf, \
             double* : vec_DotNd, \
             I32*    : vec_DotNi, \
             I64*    : vec_DotNl)(a, b, length)

/// Squared distance between two vectors of arbitrary length
#define vecDistanceSquaredN(a, b, length)     \
    _Generic((a),                             \
             float*  : vec_DistanceSquaredNf, \
             double* : vec_DistanceSquaredNd, \
             I32*    : vec_DistanceSquaredNi, \
             I64*    : vec_DistanceSquaredNl)(a, b, length)

/// Linear interpolation of two vectors of arbitrary length
#define vecLerpN(a, b, length, t, out) \
    _Generic((a),                      \
             float*  : vec_LerpNf,     \
             double* : vec_LerpNd,     \
             I32*    : vec_LerpNi,     \
             I64*    : vec_LerpNl)(a, b, length, t, out)

/// Negate a vector of arbitrary length
#define vecNegateN(v, length, out)   \
    _Generic((v),                    \
             float*  : vec_NegateNf, \
             double* : vec_NegateNd, \
             I32*    : vec_NegateNi, \
             I64*    : vec_NegateNl)(v, length, out)

/// Squared norm of a vector of arbitrary length
#define vecNormSquaredN(v, length)        \
    _Generic((v),                         \
             float*  : vec_NormSquaredNf, \
             double* : vec_NormSquaredNd, \
             I32*    : vec_NormSquaredNi, \
             I64*    : vec_NormSquaredNl)(v, length)

// clang-format on

/// Distance between two vectors of arbitrary length
#define vecDistanceN(v, length) mSqrt(vecDistanceSquaredN(v, length))

/// Norm of a vector of arbitrary length
#define vecNormN(v, length) mSqrt(vecNormSquaredN(v, length))

#define VEC__N_OPS(tag, Scalar)                                                                   \
    static inline void vec_AddN##tag(Scalar const *a, Scalar const *b, Size length, Scalar *out)  \
    {                                                                                             \
        for (Size n = 0; n < length; ++n) out[n] = a[n] + b[n];                                   \
    }                                                                                             \
                                                                                                  \
    static inline void vec_SubN##tag(Scalar const *a, Scalar const *b, Size length, Scalar *out)  \
    {                                                                                             \
        for (Size n = 0; n < length; ++n) out[n] = a[n] - b[n];                                   \
    }                                                                                             \
                                                                                                  \
    static inline void vec_MulN##tag(Scalar const *a, Scalar b, Size length, Scalar *out)         \
    {                                                                                             \
        for (Size n = 0; n < length; ++n) out[n] = a[n] * b;                                      \
    }                                                                                             \
                                                                                                  \
    static inline void vec_DivN##tag(Scalar const *a, Scalar b, Size length, Scalar *out)         \
    {                                                                                             \
        for (Size n = 0; n < length; ++n) out[n] = a[n] / b;                                      \
    }                                                                                             \
                                                                                                  \
    static inline Scalar vec_DotN##tag(Scalar const *a, Scalar const *b, Size length)             \
    {                                                                                             \
        Scalar out = 0;                                                                           \
        for (Size n = 0; n < length; ++n) out += a[n] * b[n];                                     \
        return out;                                                                               \
    }                                                                                             \
                                                                                                  \
    static inline Scalar vec_DistanceSquaredN##tag(Scalar const *a, Scalar const *b, Size length) \
    {                                                                                             \
                                                                                                  \
        Scalar out = 0;                                                                           \
        for (Size n = 0; n < length; ++n)                                                         \
        {                                                                                         \
            Scalar diff = a[n] - b[n];                                                            \
            out += diff * diff;                                                                   \
        }                                                                                         \
        return out;                                                                               \
    }                                                                                             \
                                                                                                  \
    static inline void vec_LerpN##tag(Scalar const *a, Scalar const *b, Size length, Scalar t,    \
                                      Scalar *out)                                                \
    {                                                                                             \
        Scalar t1 = (Scalar)1 - t;                                                                \
        for (Size n = 0; n < length; ++n) out[n] = t1 * a[n] + t * b[n];                          \
    }                                                                                             \
                                                                                                  \
    static inline void vec_NegateN##tag(Scalar const *v, Size length, Scalar *out)                \
    {                                                                                             \
        for (Size n = 0; n < length; ++n) out[n] = -v[n];                                         \
    }                                                                                             \
                                                                                                  \
    static inline Scalar vec_NormSquaredN##tag(Scalar const *v, Size length)                      \
    {                                                                                             \
        return vec_DotN##tag(v, v, length);                                                       \
    }

VEC__N_OPS(f, float)
VEC__N_OPS(d, double)
VEC__N_OPS(i, I32)
VEC__N_OPS(l, I64)
#undef VEC__N_OPS

//------------------------------------//
//   Type-generic vector operations   //
//------------------------------------//

#define VEC3_0 ((Vec3f){0})
#define VEC3_X ((Vec3f){.x = 1.0f, .y = 0.0f, .z = 0.0f})
#define VEC3_Y ((Vec3f){.x = 0.0f, .y = 1.0f, .z = 0.0f})
#define VEC3_Z ((Vec3f){.x = 0.0f, .y = 0.0f, .z = 1.0f})

/// Add two vectors
#define vecAdd(a, b)     \
    _Generic(((a), (b)), \
        Vec2f: vecAdd2f, \
        Vec3f: vecAdd3f, \
        Vec4f: vecAdd4f, \
        Vec2d: vecAdd2d, \
        Vec3d: vecAdd3d, \
        Vec4d: vecAdd4d, \
        Vec2i: vecAdd2i, \
        Vec3i: vecAdd3i, \
        Vec4i: vecAdd4i)(a, b)

/// Subtract two vectors
#define vecSub(a, b)     \
    _Generic(((a), (b)), \
        Vec2f: vecSub2f, \
        Vec3f: vecSub3f, \
        Vec4f: vecSub4f, \
        Vec2d: vecSub2d, \
        Vec3d: vecSub3d, \
        Vec4d: vecSub4d, \
        Vec2i: vecSub2i, \
        Vec3i: vecSub3i, \
        Vec4i: vecSub4i)(a, b)

/// Multiply a vector by a scalar
#define vecMul(a, b)     \
    _Generic((a),        \
        Vec2f: vecMul2f, \
        Vec3f: vecMul3f, \
        Vec4f: vecMul4f, \
        Vec2d: vecMul2d, \
        Vec3d: vecMul3d, \
        Vec4d: vecMul4d, \
        Vec2i: vecMul2i, \
        Vec3i: vecMul3i, \
        Vec4i: vecMul4i)(a, b)

/// Divide a vector by a scalar
#define vecDiv(a, b)     \
    _Generic((a),        \
        Vec2f: vecDiv2f, \
        Vec3f: vecDiv3f, \
        Vec4f: vecDiv4f, \
        Vec2d: vecDiv2d, \
        Vec3d: vecDiv3d, \
        Vec4d: vecDiv4d, \
        Vec2i: vecDiv2i, \
        Vec3i: vecDiv3i, \
        Vec4i: vecDiv4i)(a, b)

/// Dot (scalar) product of two vectors
#define vecDot(a, b)     \
    _Generic(((a), (b)), \
        Vec2f: vecDot2f, \
        Vec3f: vecDot3f, \
        Vec4f: vecDot4f, \
        Vec2d: vecDot2d, \
        Vec3d: vecDot3d, \
        Vec4d: vecDot4d, \
        Vec2i: vecDot2i, \
        Vec3i: vecDot3i, \
        Vec4i: vecDot4i)(a, b)

/// Squared distance between two vectors
#define vecDistanceSquared(a, b)     \
    _Generic(((a), (b)),             \
        Vec2f: vecDistanceSquared2f, \
        Vec3f: vecDistanceSquared3f, \
        Vec4f: vecDistanceSquared4f, \
        Vec2d: vecDistanceSquared2d, \
        Vec3d: vecDistanceSquared3d, \
        Vec4d: vecDistanceSquared4d, \
        Vec2i: vecDistanceSquared2i, \
        Vec3i: vecDistanceSquared3i, \
        Vec4i: vecDistanceSquared4i)(a, b)

/// Linear interpolation of two vectors
#define vecLerp(a, b, t)  \
    _Generic(((a), (b)),  \
        Vec2f: vecLerp2f, \
        Vec3f: vecLerp3f, \
        Vec4f: vecLerp4f, \
        Vec2d: vecLerp2d, \
        Vec3d: vecLerp3d, \
        Vec4d: vecLerp4d, \
        Vec2i: vecLerp2i, \
        Vec3i: vecLerp3i, \
        Vec4i: vecLerp4i)(a, b, t)

/// Negate a vector
#define vecNegate(v)        \
    _Generic((v),           \
        Vec2f: vecNegate2f, \
        Vec3f: vecNegate3f, \
        Vec4f: vecNegate4f, \
        Vec2d: vecNegate2d, \
        Vec3d: vecNegate3d, \
        Vec4d: vecNegate4d, \
        Vec2i: vecNegate2i, \
        Vec3i: vecNegate3i, \
        Vec4i: vecNegate4i)(v)

/// Squared norm of a vector
#define vecNormSquared(v)        \
    _Generic((v),                \
        Vec2f: vecNormSquared2f, \
        Vec3f: vecNormSquared3f, \
        Vec4f: vecNormSquared4f, \
        Vec2d: vecNormSquared2d, \
        Vec3d: vecNormSquared3d, \
        Vec4d: vecNormSquared4d, \
        Vec2i: vecNormSquared2i, \
        Vec3i: vecNormSquared3i, \
        Vec4i: vecNormSquared4i)(v)

/// Compute the normalized vector
#define vecNormalize(v)        \
    _Generic((v),              \
        Vec2f: vecNormalize2f, \
        Vec3f: vecNormalize3f, \
        Vec4f: vecNormalize4f, \
        Vec2d: vecNormalize2d, \
        Vec3d: vecNormalize3d, \
        Vec4d: vecNormalize4d, \
        Vec2i: vecNormalize2i, \
        Vec3i: vecNormalize3i, \
        Vec4i: vecNormalize4i)(v)

/// Distance between two vectors
#define vecDistance(a, b) mSqrt(vecDistanceSquared(a, b))

/// Norm of a vector
#define vecNorm(v) mSqrt(vecNormSquared(v))

/// The "perp dot product" a^_|_·b for a and b vectors in the plane is a modification of the
/// two-dimensional dot product in which a is replaced by the perpendicular vector rotated 90
/// degrees to the left defined by Hill (1994). It satisfies the identities
#define vecPerpDot(a, b) _Generic(((a), (b)), Vec2f: vecPerpDot2, Vec2d: vecPerpDot2d)(a, b)

/// Cross product of two 3d vectors
#define vecCross(a, b) _Generic(((a), (b)), Vec3f: vecCross3, Vec3d: vecCross3d)(a, b)

//-------------------------------------//
//   Type-specific vector operations   //
//-------------------------------------//

static inline float
vecPerpDot2(Vec2f a, Vec2f b)
{
    return a.x * b.y - a.y * b.x;
}

static inline double
vecPerpDot2d(Vec2d a, Vec2d b)
{
    return a.x * b.y - a.y * b.x;
}

static inline Vec3f
vecCross3(Vec3f a, Vec3f b)
{
    return (Vec3f){.x = a.y * b.z - a.z * b.y, //
                   .y = a.z * b.x - a.x * b.z,
                   .z = a.x * b.y - a.y * b.x};
}

static inline Vec3d
vecCross3d(Vec3d a, Vec3d b)
{
    return (Vec3d){.x = a.y * b.z - a.z * b.y, //
                   .y = a.z * b.x - a.x * b.z,
                   .z = a.x * b.y - a.y * b.x};
}

#define VEC__OPS(Scalar, N, tag)                                                      \
    static inline Vec##N##tag vecAdd##N##tag(Vec##N##tag a, Vec##N##tag b)            \
    {                                                                                 \
        Vec##N##tag out = {0};                                                        \
        vecAddN(a.elem, b.elem, N, out.elem);                                         \
        return out;                                                                   \
    }                                                                                 \
                                                                                      \
    static inline Vec##N##tag vecSub##N##tag(Vec##N##tag a, Vec##N##tag b)            \
    {                                                                                 \
        Vec##N##tag out = {0};                                                        \
        vecSubN(a.elem, b.elem, N, out.elem);                                         \
        return out;                                                                   \
    }                                                                                 \
                                                                                      \
    static inline Vec##N##tag vecMul##N##tag(Vec##N##tag a, Scalar b)                 \
    {                                                                                 \
        Vec##N##tag out = {0};                                                        \
        vecMulN(a.elem, b, N, out.elem);                                              \
        return out;                                                                   \
    }                                                                                 \
                                                                                      \
    static inline Vec##N##tag vecDiv##N##tag(Vec##N##tag a, Scalar b)                 \
    {                                                                                 \
        Vec##N##tag out = {0};                                                        \
        vecDivN(a.elem, b, N, out.elem);                                              \
        return out;                                                                   \
    }                                                                                 \
                                                                                      \
    static inline Scalar vecDot##N##tag(Vec##N##tag a, Vec##N##tag b)                 \
    {                                                                                 \
        return vecDotN(a.elem, b.elem, N);                                            \
    }                                                                                 \
                                                                                      \
    static inline Scalar vecDistanceSquared##N##tag(Vec##N##tag a, Vec##N##tag b)     \
    {                                                                                 \
        return vecDistanceSquaredN(a.elem, b.elem, N);                                \
    }                                                                                 \
                                                                                      \
    static inline Vec##N##tag vecLerp##N##tag(Vec##N##tag a, Vec##N##tag b, Scalar t) \
    {                                                                                 \
        Vec##N##tag out = {0};                                                        \
        vecLerpN(a.elem, b.elem, N, t, out.elem);                                     \
        return out;                                                                   \
    }                                                                                 \
                                                                                      \
    static inline Vec##N##tag vecNegate##N##tag(Vec##N##tag v)                        \
    {                                                                                 \
        Vec##N##tag out = {0};                                                        \
        vecNegateN(v.elem, N, out.elem);                                              \
        return out;                                                                   \
    }                                                                                 \
                                                                                      \
    static inline Scalar vecNormSquared##N##tag(Vec##N##tag v)                        \
    {                                                                                 \
        return vec_NormSquaredN##tag(v.elem, N);                                      \
    }                                                                                 \
                                                                                      \
    static inline Vec##N##tag vecNormalize##N##tag(Vec##N##tag v)                     \
    {                                                                                 \
        Scalar norm = mSqrt(vecNormSquared##N##tag(v));                               \
        return vecDiv##N##tag(v, norm);                                               \
    }

VEC__OPS(float, 2, f)
VEC__OPS(float, 3, f)
VEC__OPS(float, 4, f)

VEC__OPS(double, 2, d)
VEC__OPS(double, 3, d)
VEC__OPS(double, 4, d)

VEC__OPS(I32, 2, i)
VEC__OPS(I32, 3, i)
VEC__OPS(I32, 4, i)

#undef VEC__OPS

//-----------------------//
//   Matrix operations   //
//-----------------------//

// NOTE (Matteo): Angles are always in radians!

//--- Common forms ---//

static inline Mat4f
matDiagonal(float value)
{
    Mat4f mat = {0};
    mat.elem[0][0] = value;
    mat.elem[1][1] = value;
    mat.elem[2][2] = value;
    mat.elem[3][3] = value;
    return mat;
}

static inline Mat4f
matIdentity(void)
{
    return matDiagonal(1.0f);
}

static inline Mat4f
matTranspose(Mat4f in)
{
    Mat4f out;

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

// NOTE (Matteo): all trasformations use right hand convention, except for projections
// which convert from a RH system to a LH clip space commonly used by graphics API.

static inline Mat4f
matScale(float value)
{
    Mat4f mat = {0};
    mat.elem[0][0] = value;
    mat.elem[1][1] = value;
    mat.elem[2][2] = value;
    mat.elem[3][3] = 1.0f;
    return mat;
}

static inline Mat4f
matTranslation(float x, float y, float z)
{
    Mat4f mat = matIdentity();
    mat.elem[3][0] = x;
    mat.elem[3][1] = y;
    mat.elem[3][2] = z;
    return mat;
}

static inline Mat4f
matTranslationVec(Vec3f t)
{
    Mat4f mat = matIdentity();
    mat.cols[3].xyz = t;
    return mat;
}

static inline Mat4f
matRotation(Vec3f axis, float radians)
{
    Mat4f mat = {0};

    axis = vecNormalize(axis);

    float sintheta = mSin(radians);
    float costheta = mCos(radians);
    float cosvalue = 1.0f - costheta;

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

static inline Mat4f
matLookAt(Vec3f eye, Vec3f target, Vec3f up)
{
    Mat4f mat = {0};

    // See https://www.khronos.org/registry/OpenGL-Refpages/gl2.1/xhtml/gluLookAt.xml

    Vec3f zaxis = vecNormalize(vecSub(target, eye));
    Vec3f xaxis = vecNormalize(vecCross(zaxis, up));
    Vec3f yaxis = vecCross(xaxis, zaxis);

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

static inline Mat4f
matPerspective(float fovy, float aspect, float near_plane, float far_plane, ClipSpace clip)
{
    // See:
    // https://www.khronos.org/registry/OpenGL-Refpages/gl2.1/xhtml/gluPerspective.xml
    // http://www.songho.ca/opengl/gl_projectionmatrix.html
    // https://vincent-p.github.io/posts/vulkan_perspective_matrix

    float f = 1 / mTan(fovy / 2);
    float a = (far_plane * clip.z_far - near_plane * clip.z_near) / (near_plane - far_plane);
    float b = near_plane * (clip.z_near + a);

    Mat4f mat = {0};

    // X transform
    mat.elem[0][0] = f * aspect;
    // Y transform
    mat.elem[1][1] = f * clip.y_dir;
    // Z transform
    mat.elem[2][2] = a;
    mat.elem[3][2] = b;
    // Last row = {0,0,-1,0}
    mat.elem[2][3] = -1.0f;

    return mat;
}

static inline Mat4f
matOrtho(float width, float height, float near_plane, float far_plane, ClipSpace clip)
{
    Mat4f mat = {0};

    // X transform
    mat.elem[0][0] = 2 / width;
    // Y transform
    mat.elem[1][1] = clip.y_dir * 2 / height;
    // Z transform
    float a = (clip.z_far - clip.z_near) / (near_plane - far_plane);
    mat.elem[2][2] = a;
    mat.elem[3][2] = far_plane * a + clip.z_far;
    // Last row = {0,0,0,1}
    mat.elem[3][3] = 1;

    return mat;
}

static inline Mat4f
matOrthoBounds(float left, float right, //
               float bottom, float top, //
               float near_plane, float far_plane, ClipSpace clip)
{
    Mat4f mat = {0};

    // X transform
    mat.elem[0][0] = 2 / (right - left);
    mat.elem[3][0] = (right + left) / (left - right);
    // Y transform
    mat.elem[1][1] = clip.y_dir * 2 / (top - bottom);
    mat.elem[3][1] = (top + bottom) / (bottom - top);
    // Z transform
    float a = (clip.z_far - clip.z_near) / (near_plane - far_plane);
    mat.elem[2][2] = a;
    mat.elem[3][2] = far_plane * a + clip.z_far;
    // Last row = {0,0,0,1}
    mat.elem[3][3] = 1;

    return mat;
}

//--- Algebra ---//

// clang-format off

#define matMul(left, right) \
    _Generic(((right)),     \
        Mat4f : matMulMat4,  \
        Vec2f : matMulVec2,  \
        Vec3f : matMulVec3,  \
        Vec4f : matMulVec4)(left, right)

// clang-format on

static inline Mat4f
matMulMat4(Mat4f left, Mat4f right)
{
    // TODO (Matteo): Go wide with SIMD

    Mat4f mat = {0};

    for (U32 col = 0; col < 4; ++col)
    {
        for (U32 row = 0; row < 4; ++row)
        {
            float sum = 0;

            for (U32 cur = 0; cur < 4; ++cur)
            {
                sum += left.elem[cur][row] * right.elem[col][cur];
            }

            mat.elem[col][row] = sum;
        }
    }

    return mat;
}

static inline Vec4f
matMulVec4(Mat4f mat, Vec4f vec)
{
    Vec4f res = {0};

    // TODO (Matteo): Go wide with SIMD
    for (Size col = 0; col < 4; ++col)
    {
        res = vecAdd(res, vecMul(mat.cols[col], vec.elem[col]));
    }

    return res;
}

static inline Vec3f
matMulVec3(Mat4f mat, Vec3f vec)
{
    return matMulVec4(mat, (Vec4f){.xyz = vec}).xyz;
}

static inline Vec2f
matMulVec2(Mat4f mat, Vec2f vec)
{
    return (Vec2f){
        .x = mat.elem[0][0] * vec.x + mat.elem[1][0] * vec.y + mat.elem[3][0],
        .y = mat.elem[0][1] * vec.x + mat.elem[1][1] * vec.y + mat.elem[3][1],
    };
}

//---------------------------------//
//   Common graphics clip spaces   //
//---------------------------------//

static inline ClipSpace
mClipSpaceGl(void)
{
    // NOTE (Matteo): OpenGL has a left-handed clip space, with all coordinates
    // normalized in the [-1, 1] interval, Z-included.
    return (ClipSpace){
        .y_dir = 1,
        .z_near = -1,
        .z_far = 1,
    };
}

static inline ClipSpace
mClipSpaceD3d(void)
{
    // NOTE (Matteo): Direct3d uses a left-handed clip space, with the Z coordinate
    // normalized in the [0, 1] interval
    return (ClipSpace){
        .y_dir = 1,
        .z_near = 0,
        .z_far = 1,
    };
}

static inline ClipSpace
mClipSpaceVk(bool reverse_depth)
{
    // NOTE (Matteo): Vulkan uses a left-handed clip space, with the Z coordinate
    // normalized by default in the [0, 1] interval.
    // According to https://vincent-p.github.io/posts/vulkan_perspective_matrix, using
    // "reverse depth" gives better numerical distribution so I allow it as a parameter.

    ClipSpace clip = {.y_dir = -1};

    if (reverse_depth)
    {
        clip.z_far = 1;
    }
    else
    {
        clip.z_near = 1;
    }

    return clip;
}
