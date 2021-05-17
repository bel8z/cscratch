#ifndef FOUNDATION_VEC_H

#include "common.h"
#include "util.h"

//------------------------------------------------------------------------------
// N-dimension vector operations
//------------------------------------------------------------------------------

static inline void
vecAddN(f32 const *a, f32 const *b, size_t len, f32 *out)
{
    for (f32 const *end = out + len; out != end; ++a, ++b, ++out)
    {
        *out = *a + *b;
    }
}

static inline void
vecSubN(f32 const *a, f32 const *b, size_t len, f32 *out)
{
    for (f32 const *end = out + len; out != end; ++a, ++b, ++out)
    {
        *out = *a - *b;
    }
}

static inline void
vecMulN(f32 const *a, f32 b, size_t len, f32 *out)
{
    for (f32 const *end = out + len; out != end; ++a, ++out)
    {
        *out = *a * b;
    }
}

static inline void
vecDivN(f32 const *a, f32 b, size_t len, f32 *out)
{
    for (f32 const *end = out + len; out != end; ++a, ++out)
    {
        *out = *a / b;
    }
}

static inline void
vecNegateN(f32 const *a, size_t len, f32 *out)
{
    for (f32 const *end = out + len; out != end; ++a, ++out)
    {
        *out = -(*a);
    }
}

static inline f32
vecDotN(f32 const *a, f32 const *b, size_t len)
{
    f32 out = 0;

    for (f32 const *end = a + len; a != end; ++a, ++b)
    {
        out += *a * *b;
    }

    return out;
}

static inline f32
vecNormSquaredN(f32 const *a, size_t len)
{
    return vecDotN(a, a, len);
}

static inline f32
vecNormN(f32 const *a, size_t len)
{
    return cfSqrt(vecNormSquaredN(a, len));
}

static inline f32
vecDistanceSquaredN(f32 const *a, f32 const *b, size_t len)
{
    f32 out = 0;

    for (f32 const *end = a + len; a != end; ++a, ++b)
    {
        f32 diff = *a - *b;
        out += diff * diff;
    }

    return out;
}

static inline f32
vecDistanceN(f32 const *a, f32 const *b, size_t len)
{
    return cfSqrt(vecDistanceSquaredN(a, b, len));
}

static inline void
vecLerpN(f32 const *a, f32 const *b, size_t len, f32 t, f32 *out)
{
    f32 t1 = 1.0f - t;

    for (f32 const *end = out + len; out != end; ++a, ++b, ++out)
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

static inline f32
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
    static inline Vec##N vecMul##N(Vec##N a, f32 b)                                        \
    {                                                                                      \
        Vec##N out = {0};                                                                  \
        vecMulN(a.elem, b, N, out.elem);                                                   \
        return out;                                                                        \
    }                                                                                      \
                                                                                           \
    static inline Vec##N vecDiv##N(Vec##N a, f32 b)                                        \
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
    static inline f32 vecDot##N(Vec##N a, Vec##N b) { return vecDotN(a.elem, b.elem, N); } \
                                                                                           \
    static inline f32 vecNormSquared##N(Vec##N a) { return vecNormSquaredN(a.elem, N); }   \
                                                                                           \
    static inline f32 vecNorm##N(Vec##N a) { return vecNormN(a.elem, N); }                 \
                                                                                           \
    static inline f32 vecDistanceSquared##N(Vec##N a, Vec##N b)                            \
    {                                                                                      \
        return vecDistanceSquaredN(a.elem, b.elem, N);                                     \
    }                                                                                      \
                                                                                           \
    static inline f32 vecDistance##N(Vec##N a, Vec##N b)                                   \
    {                                                                                      \
        return vecDistanceN(a.elem, b.elem, N);                                            \
    }                                                                                      \
                                                                                           \
    static inline Vec##N vecLerp##N(Vec##N a, Vec##N b, f32 t)                             \
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

#define FOUNDATION_VEC_H
#endif
