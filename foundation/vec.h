
#include "maths.h"

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
vec_add(f32 const *a, f32 const *b, size_t len, f32 *out)
{
    for (f32 const *end = out + len; out != end; ++a, ++b, ++out)
    {
        *out = *a + *b;
    }
}

static inline void
vec_sub(f32 const *a, f32 const *b, size_t len, f32 *out)
{
    for (f32 const *end = out + len; out != end; ++a, ++b, ++out)
    {
        *out = *a - *b;
    }
}

static inline void
vec_mul(f32 const *a, f32 b, size_t len, f32 *out)
{
    for (f32 const *end = out + len; out != end; ++a, ++out)
    {
        *out = *a * b;
    }
}

static inline void
vec_div(f32 const *a, f32 b, size_t len, f32 *out)
{
    for (f32 const *end = out + len; out != end; ++a, ++out)
    {
        *out = *a / b;
    }
}

static inline void
vec_negate(f32 const *a, size_t len, f32 *out)
{
    for (f32 const *end = out + len; out != end; ++a, ++out)
    {
        *out = -(*a);
    }
}

static inline f32
vec_dot(f32 const *a, f32 const *b, size_t len)
{
    f32 out = 0;

    for (f32 const *end = a + len; a != end; ++a, ++b)
    {
        out += *a * *b;
    }

    return out;
}

static inline f32
vec_norm_squared(f32 const *a, size_t len)
{
    return vec_dot(a, a, len);
}

static inline f32
vec_norm(f32 const *a, size_t len)
{
    return sqrt(vec_norm_squared(a, len));
}

static inline f32
vec_distance_squared(f32 const *a, f32 const *b, size_t len)
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
vec_distance(f32 const *a, f32 const *b, size_t len)
{
    return sqrt(vec_distance_squared(a, b, len));
}

static inline void
vec_lerp(f32 const *a, f32 const *b, size_t len, f32 t, f32 *out)
{
    f32 t1 = 1.0 - t;

    for (f32 const *end = out + len; out != end; ++a, ++b, ++out)
    {
        *out = *a * t1 + *b * t;
    }
}

static inline f32
vec2_dotperp(Vec2 a, Vec2 b)
{
    return a.x * b.y - a.y * b.x;
}

static inline Vec3
vec3_cross(Vec3 a, Vec3 b)
{
    return (Vec3){
        .x = a.y * b.z - a.z + b.y, .y = a.z * b.x - a.x * b.z, .z = a.x * b.y - a.y * b.x};
}

#define VEC_OPS(N)                                                                            \
    static inline Vec##N vec##N##_add(Vec##N a, Vec##N b)                                     \
    {                                                                                         \
        Vec##N out = {0};                                                                     \
        vec_add(a.elem, b.elem, N, out.elem);                                                 \
        return out;                                                                           \
    }                                                                                         \
                                                                                              \
    static inline Vec##N vec##N##_sub(Vec##N a, Vec##N b)                                     \
    {                                                                                         \
        Vec##N out = {0};                                                                     \
        vec_sub(a.elem, b.elem, N, out.elem);                                                 \
        return out;                                                                           \
    }                                                                                         \
                                                                                              \
    static inline Vec##N vec##N##_mul(Vec##N a, f32 b)                                        \
    {                                                                                         \
        Vec##N out = {0};                                                                     \
        vec_mul(a.elem, b, N, out.elem);                                                      \
        return out;                                                                           \
    }                                                                                         \
                                                                                              \
    static inline Vec##N vec##N##_div(Vec##N a, f32 b)                                        \
    {                                                                                         \
        Vec##N out = {0};                                                                     \
        vec_div(a.elem, b, N, out.elem);                                                      \
        return out;                                                                           \
    }                                                                                         \
                                                                                              \
    static inline f32 vec##N##_dot(Vec##N a, Vec##N b) { return vec_dot(a.elem, b.elem, N); } \
                                                                                              \
    static inline f32 vec##N##_norm_squared(Vec##N a) { return vec_norm_squared(a.elem, N); } \
                                                                                              \
    static inline f32 vec##N##_norm(Vec##N a) { return vec_norm(a.elem, N); }                 \
                                                                                              \
    static inline f32 vec##N##_distance_squared(Vec##N a, Vec##N b)                           \
    {                                                                                         \
        return vec_distance_squared(a.elem, b.elem, N);                                       \
    }                                                                                         \
                                                                                              \
    static inline f32 vec##N##_distance(Vec##N a, Vec##N b)                                   \
    {                                                                                         \
        return vec_distance(a.elem, b.elem, N);                                               \
    }                                                                                         \
                                                                                              \
    static inline Vec##N vec##N##_lerp(Vec##N a, Vec##N b, f32 t)                             \
    {                                                                                         \
        Vec##N out = {0};                                                                     \
        vec_lerp(a.elem, b.elem, N, t, out.elem);                                             \
        return out;                                                                           \
    }

VEC_OPS(2)
VEC_OPS(3)
VEC_OPS(4)

// Mat 4
// TODO
