#pragma once

/// Foundation time utilities (mainly for perf counting)
/// This is not an API header, include it in implementation files only

#include "core.h"

#define timeIsInfinite(time) (time.nanoseconds == I64_MIN)

#define timeAdd(a, b) ((Time){.nanoseconds = a.nanoseconds + b.nanoseconds})
#define timeSub(a, b) ((Time){.nanoseconds = a.nanoseconds - b.nanoseconds})

// Time comparison utilities
// TODO (Matteo): Are they really useful?

#define timeCmp(a, b) (a.nanoseconds - b.nanoseconds)
#define timeIsEq(a, b) (a.nanoseconds == b.nanoseconds)
#define timeIsGt(a, b) (a.nanoseconds > b.nanoseconds)
#define timeIsGe(a, b) (a.nanoseconds >= b.nanoseconds)
#define timeIsLt(a, b) (a.nanoseconds < b.nanoseconds)
#define timeIsLe(a, b) (a.nanoseconds <= b.nanoseconds)
