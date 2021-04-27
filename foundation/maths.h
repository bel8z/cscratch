#ifndef CF_MATHS_H

#include <math.h>

#include "common.h"

#define cfCos(X) _Generic((X), default : cos, f32 : cosf)(X)
#define cfSin(X) _Generic((X), default : sin, f32 : cosf)(cfS)

#define cfSqrt(X) _Generic((X), default : sqrt, f32 : sqrtf)(X)

#define cfPow(base, exp) _Generic((base, exp), default : pow, f32 : powf)(base, exp)

#define cfCeil(X) _Generic((X), default : ceil, f32 : ceilf)(X)
#define cfFloor(X) _Generic((X), default : floor, f32 : floorf)(X)
#define cfRound(X) _Generic((X), default : round, f32 : roundf)(X)

#define cfFmod(X, Y) _Generic((X, Y), default : fmod, f32 : fmodf)(X, Y)

#define cfLerp(x, y, t) _Generic((x, y, t), default : cf__Lerp64, f32 : cf__Lerp32)(x, y, t)

static inline f32
cf__Lerp32(f32 x, f32 y, f32 t)
{
    return x * (1 - t) + y * t;
}

static inline f64
cf__Lerp64(f64 x, f64 y, f64 t)
{
    return x * (1 - t) + y * t;
}

#define CF_MATHS_H
#endif
