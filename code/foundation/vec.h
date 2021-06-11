#pragma once

#include "common.h"
#include "util.h"

//------------------------------------------------------------------------------
// N-dimension vector operations
//------------------------------------------------------------------------------

static inline void
vecAddN(F32 const *a, F32 const *b, size_t len, F32 *out)
{
    for (F32 const *end = out + len; out != end; ++a, ++b, ++out)
    {
        *out = *a + *b;
    }
}

static inline void
vecSubN(F32 const *a, F32 const *b, size_t len, F32 *out)
{
    for (F32 const *end = out + len; out != end; ++a, ++b, ++out)
    {
        *out = *a - *b;
    }
}

static inline void
vecMulN(F32 const *a, F32 b, size_t len, F32 *out)
{
    for (F32 const *end = out + len; out != end; ++a, ++out)
    {
        *out = *a * b;
    }
}

static inline void
vecDivN(F32 const *a, F32 b, size_t len, F32 *out)
{
    for (F32 const *end = out + len; out != end; ++a, ++out)
    {
        *out = *a / b;
    }
}

static inline void
vecNegateN(F32 const *a, size_t len, F32 *out)
{
    for (F32 const *end = out + len; out != end; ++a, ++out)
    {
        *out = -(*a);
    }
}

static inline F32
vecDotN(F32 const *a, F32 const *b, size_t len)
{
    F32 out = 0;

    for (F32 const *end = a + len; a != end; ++a, ++b)
    {
        out += *a * *b;
    }

    return out;
}

static inline F32
vecNormSquaredN(F32 const *a, size_t len)
{
    return vecDotN(a, a, len);
}

static inline F32
vecNormN(F32 const *a, size_t len)
{
    return cfSqrt(vecNormSquaredN(a, len));
}

static inline F32
vecDistanceSquaredN(F32 const *a, F32 const *b, size_t len)
{
    F32 out = 0;

    for (F32 const *end = a + len; a != end; ++a, ++b)
    {
        F32 diff = *a - *b;
        out += diff * diff;
    }

    return out;
}

static inline F32
vecDistanceN(F32 const *a, F32 const *b, size_t len)
{
    return cfSqrt(vecDistanceSquaredN(a, b, len));
}

static inline void
vecLerpN(F32 const *a, F32 const *b, size_t len, F32 t, F32 *out)
{
    F32 t1 = 1.0f - t;

    for (F32 const *end = out + len; out != end; ++a, ++b, ++out)
    {
        *out = *a * t1 + *b * t;
    }
}

//------------------------------------------------------------------------------
// Type-generic vector operations
//------------------------------------------------------------------------------

#define vecAdd(a, b) _Generic((a, b), Vec2 : vecAdd2, Vec3 : vecAdd3, Vec4 : vecAdd4)(a, b)
#define vecSub(a, b) _Generic((a, b), Vec2 : vecSub2, Vec3 : vecSub3, Vec4 : vecSub4)(a, b)
#define vecMul(a, b) _Generic((a), Vec2 : vecMul2, Vec3 : vecMul3, Vec4 : vecMul4)(a, b)
#define vecDiv(a, b) _Generic((a), Vec2 : vecDiv2, Vec3 : vecDiv3, Vec4 : vecDiv4)(a, b)

#define vecNegate(a) _Generic((a), Vec2 : vecNegate2, Vec3 : vecNegate3, Vec4 : vecNegate4)(a)

#define vecDot(a, b) _Generic((a, b), Vec2 : vecDot2, Vec3 : vecDot3, Vec4 : vecDot4)(a, b)

#define vecNormSquared(a) \
    _Generic((a), Vec2 : vecNormSquared2, Vec3 : vecNormSquared3, Vec4 : vecNormSquared4)(a)

#define vecNorm(a) _Generic((a), Vec2 : vecNorm2, Vec3 : vecNorm3, Vec4 : vecNorm4)(a)

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

//------------------------------------------------------------------------------
// Type-specific vector operations
//------------------------------------------------------------------------------

static inline F32
vecDotPerp2(Vec2 a, Vec2 b)
{
    return a.x * b.y - a.y * b.x;
}

static inline Vec3
vecCross3(Vec3 a, Vec3 b)
{
    return (Vec3){
        .x = a.y * b.z - a.z + b.y, .y = a.z * b.x - a.x * b.z, .z = a.x * b.y - a.y * b.x};
}

#define VEC_OPS(N)                                                                         \
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

VEC_OPS(2)
VEC_OPS(3)
VEC_OPS(4)

#undef VEC_OPS

// Mat 4
// TODO
