#ifndef FOUNDATION_VEC_H

#include "common.h"
#include "util.h"

typedef union Vec2 Vec2;
typedef union Vec3 Vec3;
typedef union Vec4 Vec4;
typedef struct Mat4 Mat4;

union Vec2
{
    struct
    {
        f32 x, y;
    };
    struct
    {
        f32 u, v;
    };
    f32 elem[2];
};

union Vec3
{
    struct
    {
        f32 x, y, z;
    };
    struct
    {
        f32 r, g, b;
    };
    f32 elem[3];
};

union Vec4
{
    struct
    {
        f32 x, y, z, w;
    };
    struct
    {
        f32 r, g, b, a;
    };
    f32 elem[4];
};

struct Mat4
{
    f32 elem[4][4];
};

static inline void
vecAdd(f32 const *a, f32 const *b, size_t len, f32 *out)
{
    for (f32 const *end = out + len; out != end; ++a, ++b, ++out)
    {
        *out = *a + *b;
    }
}

static inline void
vecSub(f32 const *a, f32 const *b, size_t len, f32 *out)
{
    for (f32 const *end = out + len; out != end; ++a, ++b, ++out)
    {
        *out = *a - *b;
    }
}

static inline void
vecMul(f32 const *a, f32 b, size_t len, f32 *out)
{
    for (f32 const *end = out + len; out != end; ++a, ++out)
    {
        *out = *a * b;
    }
}

static inline void
vecDiv(f32 const *a, f32 b, size_t len, f32 *out)
{
    for (f32 const *end = out + len; out != end; ++a, ++out)
    {
        *out = *a / b;
    }
}

static inline void
vecNegate(f32 const *a, size_t len, f32 *out)
{
    for (f32 const *end = out + len; out != end; ++a, ++out)
    {
        *out = -(*a);
    }
}

static inline f32
vecDot(f32 const *a, f32 const *b, size_t len)
{
    f32 out = 0;

    for (f32 const *end = a + len; a != end; ++a, ++b)
    {
        out += *a * *b;
    }

    return out;
}

static inline f32
vecNormSquared(f32 const *a, size_t len)
{
    return vecDot(a, a, len);
}

static inline f32
vecNorm(f32 const *a, size_t len)
{
    return cfSqrt(vecNormSquared(a, len));
}

static inline f32
vecDistanceSquared(f32 const *a, f32 const *b, size_t len)
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
vecDistance(f32 const *a, f32 const *b, size_t len)
{
    return cfSqrt(vecDistanceSquared(a, b, len));
}

static inline void
vecLerp(f32 const *a, f32 const *b, size_t len, f32 t, f32 *out)
{
    f32 t1 = 1.0f - t;

    for (f32 const *end = out + len; out != end; ++a, ++b, ++out)
    {
        *out = *a * t1 + *b * t;
    }
}

static inline f32
vec2DotPerp(Vec2 a, Vec2 b)
{
    return a.x * b.y - a.y * b.x;
}

static inline Vec3
vec3Cross(Vec3 a, Vec3 b)
{
    return (Vec3){
        .x = a.y * b.z - a.z + b.y, .y = a.z * b.x - a.x * b.z, .z = a.x * b.y - a.y * b.x};
}

#define VEC_OPS(N)                                                                          \
    static inline Vec##N vec##N##Add(Vec##N a, Vec##N b)                                    \
    {                                                                                       \
        Vec##N out = {0};                                                                   \
        vecAdd(a.elem, b.elem, N, out.elem);                                                \
        return out;                                                                         \
    }                                                                                       \
                                                                                            \
    static inline Vec##N vec##N##Sub(Vec##N a, Vec##N b)                                    \
    {                                                                                       \
        Vec##N out = {0};                                                                   \
        vecSub(a.elem, b.elem, N, out.elem);                                                \
        return out;                                                                         \
    }                                                                                       \
                                                                                            \
    static inline Vec##N vec##N##Mul(Vec##N a, f32 b)                                       \
    {                                                                                       \
        Vec##N out = {0};                                                                   \
        vecMul(a.elem, b, N, out.elem);                                                     \
        return out;                                                                         \
    }                                                                                       \
                                                                                            \
    static inline Vec##N vec##N##Div(Vec##N a, f32 b)                                       \
    {                                                                                       \
        Vec##N out = {0};                                                                   \
        vecDiv(a.elem, b, N, out.elem);                                                     \
        return out;                                                                         \
    }                                                                                       \
                                                                                            \
    static inline f32 vec##N##Dot(Vec##N a, Vec##N b) { return vecDot(a.elem, b.elem, N); } \
                                                                                            \
    static inline f32 vec##N##NormSquared(Vec##N a) { return vecNormSquared(a.elem, N); }   \
                                                                                            \
    static inline f32 vec##N##Norm(Vec##N a) { return vecNorm(a.elem, N); }                 \
                                                                                            \
    static inline f32 vec##N##DistanceSquared(Vec##N a, Vec##N b)                           \
    {                                                                                       \
        return vecDistanceSquared(a.elem, b.elem, N);                                       \
    }                                                                                       \
                                                                                            \
    static inline f32 vec##N##Distance(Vec##N a, Vec##N b)                                  \
    {                                                                                       \
        return vecDistance(a.elem, b.elem, N);                                              \
    }                                                                                       \
                                                                                            \
    static inline Vec##N vec##N##Lerp(Vec##N a, Vec##N b, f32 t)                            \
    {                                                                                       \
        Vec##N out = {0};                                                                   \
        vecLerp(a.elem, b.elem, N, t, out.elem);                                            \
        return out;                                                                         \
    }

VEC_OPS(2)
VEC_OPS(3)
VEC_OPS(4)

#undef VEC_OPS

// Mat 4
// TODO

#define FOUNDATION_VEC_H
#endif
