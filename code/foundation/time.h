#pragma once

/// Foundation time utilities (mainly for perf counting)
/// This is not an API header, include it in implementation files only

#include "core.h"

#define timeIsInfinite(time) (time.nanoseconds == I64_MIN)

#define timeAdd(a, b) ((Time){.nanoseconds = a.nanoseconds + b.nanoseconds})
#define timeSub(a, b) ((Time){.nanoseconds = a.nanoseconds - b.nanoseconds})
