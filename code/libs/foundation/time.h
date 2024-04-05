#pragma once

/// Foundation time utilities (mainly for perf counting)
/// This is an API header, only "core.h" can be included

#include "core.h"

//--------------------------------//
//   Monotonic time measurement   //
//--------------------------------//

typedef struct Clock
{
    U8 opaque[16];
} Clock;

void clockStart(Clock *clock);
Duration clockElapsed(Clock *clock);

//---------------------------//
//   System time utilities   //
//---------------------------//

typedef U8 WeekDayU8;
enum WeekDay_
{
    WeekDay_Sun = 0,
    WeekDay_Mon,
    WeekDay_Tue,
    WeekDay_Wed,
    WeekDay_Thu,
    WeekDay_Fri,
    WeekDay_Sat,
};

typedef struct CalendarTime
{
    U16 year;
    U8 month; // Ordinal [1-12]
    U8 day;   // Ordinal [1-31]
    WeekDayU8 week_day;
    U8 hour;
    U8 minute;
    U8 second;
    U16 milliseconds;
} CalendarTime;

SystemTime timeGetSystem(void);
CalendarTime timeGetUtc(SystemTime sys_time);
CalendarTime timeGetLocal(SystemTime sys_time);

//------------------------------//
//   Time duration operations   //
//------------------------------//

Duration timeDurationSecs(I64 seconds);
Duration timeDurationMs(U64 milliseconds);
Duration timeDurationUs(U64 microseconds);
Duration timeDurationNs(U64 nanoseconds);

double timeGetSeconds(Duration duration);

//=== Comparison ===//

bool timeIsInfinite(Duration d);

bool timeIsEq(Duration lhs, Duration rhs);
bool timeIsGt(Duration lhs, Duration rhs);
bool timeIsGe(Duration lhs, Duration rhs);
bool timeIsLt(Duration lhs, Duration rhs);
bool timeIsLe(Duration lhs, Duration rhs);

//=== Arithmetic ===//

Duration timeAdd(Duration lhs, Duration rhs);
Duration timeSub(Duration lhs, Duration rhs);
