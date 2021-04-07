
#include <math.h>
#include <stddef.h>

typedef union Vec2 Vec2;
typedef union Vec3 Vec3;
typedef union Vec4 Vec4;
typedef struct Mat4 Mat4;

typedef double Real;

#define sqrt(X)                                                                \
    _Generic((X), long double : sqrtl, default : sqrt, float : sqrtf)(X)

union Vec2
{
    struct
    {
        Real x, y;
    };
    struct
    {
        Real u, v;
    };
    Real elem[2];
};

union Vec3
{
    struct
    {
        Real x, y, z;
    };
    struct
    {
        Real r, g, b;
    };
    Real elem[3];
};

union Vec4
{
    struct
    {
        Real x, y, z, w;
    };
    struct
    {
        Real r, g, b, a;
    };
    Real elem[4];
};

struct Mat4
{
    Real elem[4][4];
};

static inline void
vec_add(Real const *a, Real const *b, size_t len, Real *out)
{
    for (Real const *end = out + len; out != end; ++a, ++b, ++out)
    {
        *out = *a + *b;
    }
}

static inline void
vec_sub(Real const *a, Real const *b, size_t len, Real *out)
{
    for (Real const *end = out + len; out != end; ++a, ++b, ++out)
    {
        *out = *a - *b;
    }
}

static inline void
vec_mul(Real const *a, Real b, size_t len, Real *out)
{
    for (Real const *end = out + len; out != end; ++a, ++out)
    {
        *out = *a * b;
    }
}

static inline void
vec_div(Real const *a, Real b, size_t len, Real *out)
{
    for (Real const *end = out + len; out != end; ++a, ++out)
    {
        *out = *a / b;
    }
}

static inline void
vec_negate(Real const *a, size_t len, Real *out)
{
    for (Real const *end = out + len; out != end; ++a, ++out)
    {
        *out = -(*a);
    }
}

static inline Real
vec_dot(Real const *a, Real const *b, size_t len)
{
    Real out = 0;

    for (Real const *end = a + len; a != end; ++a, ++b)
    {
        out += *a * *b;
    }

    return out;
}

static inline Real
vec_norm_squared(Real const *a, size_t len)
{
    return vec_dot(a, a, len);
}

static inline Real
vec_norm(Real const *a, size_t len)
{
    return sqrt(vec_norm_squared(a, len));
}

static inline Real
vec_distance_squared(Real const *a, Real const *b, size_t len)
{
    Real out = 0;

    for (Real const *end = a + len; a != end; ++a, ++b)
    {
        Real diff = *a - *b;
        out += diff * diff;
    }

    return out;
}

static inline Real
vec_distance(Real const *a, Real const *b, size_t len)
{
    return sqrt(vec_distance_squared(a, b, len));
}

static inline void
vec_lerp(Real const *a, Real const *b, size_t len, Real t, Real *out)
{
    Real t1 = 1.0 - t;

    for (Real const *end = out + len; out != end; ++a, ++b, ++out)
    {
        *out = *a * t1 + *b * t;
    }
}

static inline Real
vec2_dotperp(Vec2 a, Vec2 b)
{
    return a.x * b.y - a.y * b.x;
}

static inline Vec3
vec3_cross(Vec3 a, Vec3 b)
{
    return (Vec3){.x = a.y * b.z - a.z + b.y,
                  .y = a.z * b.x - a.x * b.z,
                  .z = a.x * b.y - a.y * b.x};
}

#define VEC_OPS(N)                                                             \
    static inline Vec##N vec##N##_add(Vec##N a, Vec##N b)                      \
    {                                                                          \
        Vec##N out = {0};                                                      \
        vec_add(a.elem, b.elem, N, out.elem);                                  \
        return out;                                                            \
    }                                                                          \
                                                                               \
    static inline Vec##N vec##N##_sub(Vec##N a, Vec##N b)                      \
    {                                                                          \
        Vec##N out = {0};                                                      \
        vec_sub(a.elem, b.elem, N, out.elem);                                  \
        return out;                                                            \
    }                                                                          \
                                                                               \
    static inline Vec##N vec##N##_mul(Vec##N a, Real b)                        \
    {                                                                          \
        Vec##N out = {0};                                                      \
        vec_mul(a.elem, b, N, out.elem);                                       \
        return out;                                                            \
    }                                                                          \
                                                                               \
    static inline Vec##N vec##N##_div(Vec##N a, Real b)                        \
    {                                                                          \
        Vec##N out = {0};                                                      \
        vec_div(a.elem, b, N, out.elem);                                       \
        return out;                                                            \
    }                                                                          \
                                                                               \
    static inline Real vec##N##_dot(Vec##N a, Vec##N b)                        \
    {                                                                          \
        return vec_dot(a.elem, b.elem, N);                                     \
    }                                                                          \
                                                                               \
    static inline Real vec##N##_norm_squared(Vec##N a)                         \
    {                                                                          \
        return vec_norm_squared(a.elem, N);                                    \
    }                                                                          \
                                                                               \
    static inline Real vec##N##_norm(Vec##N a) { return vec_norm(a.elem, N); } \
                                                                               \
    static inline Real vec##N##_distance_squared(Vec##N a, Vec##N b)           \
    {                                                                          \
        return vec_distance_squared(a.elem, b.elem, N);                        \
    }                                                                          \
                                                                               \
    static inline Real vec##N##_distance(Vec##N a, Vec##N b)                   \
    {                                                                          \
        return vec_distance(a.elem, b.elem, N);                                \
    }                                                                          \
                                                                               \
    static inline Vec##N vec##N##_lerp(Vec##N a, Vec##N b, Real t)             \
    {                                                                          \
        Vec##N out = {0};                                                      \
        vec_lerp(a.elem, b.elem, N, t, out.elem);                              \
        return out;                                                            \
    }

VEC_OPS(2)
VEC_OPS(3)
VEC_OPS(4)

// Mat 4
// TODO

#include <stdio.h>

void
print_vec(Vec2 v)
{
    printf("{%f;%f}\n", v.x, v.y);
}

int
main(void)
{
    Vec2 a = {{1, 1}};
    Vec2 b = {{-3, -5}};
    Vec2 c = vec2_add(a, b);
    Vec2 d = vec2_div(c, 2.0);

    print_vec(a);
    print_vec(b);
    print_vec(c);
    print_vec(d);
}
