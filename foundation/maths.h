#ifndef MATHS_H

#include <math.h>

#include "common.h"

#define cos(X) _Generic((X), default : cos, f32 : cosf)(X)
#define sin(X) _Generic((X), default : sin, f32 : cosf)(X)

#define sqrt(X) _Generic((X), default : sqrt, f32 : sqrtf)(X)

#define round(X) _Generic((X), default : round, f32 : roundf)(X)

#define fmod(X, Y) _Generic((X, Y), default : fmod, f32 : fmodf)(X, Y)

#define MATHS_H
#endif
