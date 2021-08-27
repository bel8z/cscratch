#pragma once

/// Foundation time utilities (mainly for perf counting)
/// This is not an API header, include it in implementation files only

#include "core.h"

//---------//
//   API   //
//---------//

typedef struct CalendarTime
{
    U16 year;
    U8 month;
    U8 day;
    U8 week_day;
    U8 hour;
    U8 minute;
    U8 second;
    U16 milliseconds;
} CalendarTime;

typedef struct CfTimeApi
{
    /// Elapsed time since the start of the application
    /// Useful for performance measurement
    Duration (*clock)(void);

    SystemTime (*systemTime)(void);

    CalendarTime (*utcTime)(SystemTime sys_time);
    CalendarTime (*localTime)(SystemTime sys_time);
} CfTimeApi;

//---------------//
//   Utilities   //
//---------------//

#define CF_NS_PER_SEC (U64)(1000000000)
#define CF_NS_PER_MS (U64)(1000000)
#define CF_NS_PER_US (U64)(1000)
#define CF_MS_PER_SEC (U64)(1000)
#define CF_US_PER_SEC (U64)(1000000)

static inline Duration
timeDurationSecs(I64 seconds)
{
    return (Duration){.seconds = seconds};
}

static inline Duration
timeDurationMs(U64 milliseconds)
{
    return (Duration){
        .seconds = milliseconds / CF_MS_PER_SEC,
        .nanos = (U32)(milliseconds % CF_MS_PER_SEC) * CF_NS_PER_MS,
    };
}

static inline Duration
timeDurationUs(U64 microseconds)
{
    return (Duration){
        .seconds = microseconds / CF_US_PER_SEC,
        .nanos = (U32)(microseconds % CF_US_PER_SEC) * CF_NS_PER_US,
    };
}

static inline Duration
timeDurationNs(U64 nanoseconds)
{
    return (Duration){
        .seconds = nanoseconds / CF_NS_PER_SEC,
        .nanos = (U32)(nanoseconds % CF_NS_PER_SEC),
    };
}

//----------------//
//   Comparison   //
//----------------//

static inline bool
timeDurationIsInfinite(Duration d)
{
    return (U32_MAX == d.nanos);
}

static inline bool
timeIsEq(Duration lhs, Duration rhs)
{
    return lhs.seconds == rhs.seconds && lhs.nanos == rhs.nanos;
}

static inline bool
timeIsGt(Duration lhs, Duration rhs)
{
    I64 delta_secs = rhs.seconds - lhs.seconds;
    return delta_secs == 0 ? lhs.nanos > rhs.nanos : delta_secs < 0;
}

static inline bool
timeIsGe(Duration lhs, Duration rhs)
{
    I64 delta_secs = rhs.seconds - lhs.seconds;
    return delta_secs == 0 ? lhs.nanos >= rhs.nanos : delta_secs < 0;
}

static inline bool
timeIsLt(Duration lhs, Duration rhs)
{
    I64 delta_secs = rhs.seconds - lhs.seconds;
    return delta_secs == 0 ? lhs.nanos < rhs.nanos : delta_secs > 0;
}

static inline bool
timeIsLe(Duration lhs, Duration rhs)
{
    I64 delta_secs = rhs.seconds - lhs.seconds;
    return delta_secs == 0 ? lhs.nanos <= rhs.nanos : delta_secs > 0;
}

//----------------//
//   Arithmetic   //
//----------------//

static inline Duration
timeAdd(Duration lhs, Duration rhs)
{
    Duration sum = {.seconds = lhs.seconds + rhs.seconds, .nanos = lhs.nanos + rhs.nanos};

    if (sum.nanos > CF_NS_PER_SEC)
    {
        sum.nanos -= CF_NS_PER_SEC;
        ++sum.seconds;
    }

    return sum;
}

static inline Duration
timeSub(Duration lhs, Duration rhs)
{
    Duration diff = {.seconds = lhs.seconds - rhs.seconds};

    if (lhs.nanos >= rhs.nanos)
    {
        diff.nanos = lhs.nanos - rhs.nanos;
    }
    else
    {
        --diff.seconds;
        diff.nanos = lhs.nanos + CF_NS_PER_SEC - rhs.nanos;
    }

    return diff;
}
