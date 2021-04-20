#ifndef CF_MATHS_H

#include <math.h>

#include "common.h"

#define cos(X) _Generic((X), default : cos, f32 : cosf)(X)
#define sin(X) _Generic((X), default : sin, f32 : cosf)(X)

#define sqrt(X) _Generic((X), default : sqrt, f32 : sqrtf)(X)

#define ceil(X) _Generic((X), default : ceil, f32 : ceilf)(X)
#define floor(X) _Generic((X), default : floor, f32 : floorf)(X)
#define round(X) _Generic((X), default : round, f32 : roundf)(X)

#define fmod(X, Y) _Generic((X, Y), default : fmod, f32 : fmodf)(X, Y)

#define CF_MATHS_H
#endif
