#include "time.h"

//------------------------------//
//   Time duration operations   //
//------------------------------//

#define CF_NS_PER_SEC (U64)(1000000000)
#define CF_NS_PER_MS (U64)(1000000)
#define CF_NS_PER_US (U64)(1000)
#define CF_MS_PER_SEC (U64)(1000)
#define CF_US_PER_SEC (U64)(1000000)

Duration
timeDurationSecs(I64 seconds)
{
    return (Duration){.seconds = seconds};
}

Duration
timeDurationMs(U64 milliseconds)
{
    return (Duration){
        .seconds = milliseconds / CF_MS_PER_SEC,
        .nanos = (U32)(milliseconds % CF_MS_PER_SEC) * CF_NS_PER_MS,
    };
}

Duration
timeDurationUs(U64 microseconds)
{
    return (Duration){
        .seconds = microseconds / CF_US_PER_SEC,
        .nanos = (U32)(microseconds % CF_US_PER_SEC) * CF_NS_PER_US,
    };
}

Duration
timeDurationNs(U64 nanoseconds)
{
    return (Duration){
        .seconds = nanoseconds / CF_NS_PER_SEC,
        .nanos = (U32)(nanoseconds % CF_NS_PER_SEC),
    };
}

F64
timeGetSeconds(Duration duration)
{
    return (F64)duration.seconds + (F64)duration.nanos / CF_NS_PER_SEC;
}

bool
timeDurationIsInfinite(Duration d)
{
    return (U32_MAX == d.nanos);
}

bool
timeIsEq(Duration lhs, Duration rhs)
{
    return lhs.seconds == rhs.seconds && lhs.nanos == rhs.nanos;
}

bool
timeIsGt(Duration lhs, Duration rhs)
{
    I64 delta_secs = rhs.seconds - lhs.seconds;
    return delta_secs == 0 ? lhs.nanos > rhs.nanos : delta_secs < 0;
}

bool
timeIsGe(Duration lhs, Duration rhs)
{
    I64 delta_secs = rhs.seconds - lhs.seconds;
    return delta_secs == 0 ? lhs.nanos >= rhs.nanos : delta_secs < 0;
}

bool
timeIsLt(Duration lhs, Duration rhs)
{
    I64 delta_secs = rhs.seconds - lhs.seconds;
    return delta_secs == 0 ? lhs.nanos < rhs.nanos : delta_secs > 0;
}

bool
timeIsLe(Duration lhs, Duration rhs)
{
    I64 delta_secs = rhs.seconds - lhs.seconds;
    return delta_secs == 0 ? lhs.nanos <= rhs.nanos : delta_secs > 0;
}

Duration
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

Duration
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

//--------------------------//
//   OS-specific services   //
//--------------------------//

#ifdef CF_OS_WIN32

#    include "math.inl"
#    include "win32.inl"

typedef struct Win32Clock
{
    U64 start_ticks;
    U64 freq;
} Win32Clock;

static_assert(sizeof(Clock) == sizeof(Win32Clock), "Clock type is too small on Win32");

static CalendarTime
win32CalendarTime(SYSTEMTIME const *out)
{
    return (CalendarTime){.year = out->wYear,
                          .month = (U8)out->wMonth,
                          .day = (U8)out->wDay,
                          .week_day = (U8)out->wDayOfWeek,
                          .hour = (U8)out->wHour,
                          .minute = (U8)out->wMinute,
                          .second = (U8)out->wSecond,
                          .milliseconds = out->wMilliseconds};
}

void
clockStart(Clock *clock)
{
    CF_ASSERT_NOT_NULL(clock);

    Win32Clock *self = (Win32Clock *)clock->opaque;
    CF_ASSERT_NOT_NULL(self);

    LARGE_INTEGER now, freq;
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&now);
    CF_ASSERT(freq.QuadPart > 0, "System monotonic clock is not available");
    CF_ASSERT(now.QuadPart > 0, "System monotonic clock is not available");
    self->start_ticks = (U64)now.QuadPart;
    self->freq = (U64)freq.QuadPart;
}

Duration
clockElapsed(Clock *clock)
{
    CF_ASSERT_NOT_NULL(clock);

    Win32Clock *self = (Win32Clock *)clock->opaque;
    CF_ASSERT_NOT_NULL(self);

    LARGE_INTEGER now;
    QueryPerformanceCounter(&now);

    CF_ASSERT(now.QuadPart >= 0, "QueryPerformanceCounter returned negative ticks");

    U64 curr_ticks = (U64)(now.QuadPart);
    CF_ASSERT(curr_ticks > self->start_ticks, "QueryPerformanceCounter wrapped around");

    U64 nanos = mMulDiv((curr_ticks - self->start_ticks), CF_NS_PER_SEC, self->freq);

    return timeDurationNs(nanos);
}

SystemTime
systemTime(void)
{
    FILETIME time;
    GetSystemTimePreciseAsFileTime(&time);

    ULARGE_INTEGER temp = {.HighPart = time.dwHighDateTime, //
                           .LowPart = time.dwLowDateTime};
    return (U64)temp.QuadPart;
}

CalendarTime
utcTime(SystemTime sys_time)
{
    ULARGE_INTEGER temp = {.QuadPart = sys_time};
    FILETIME in = {.dwLowDateTime = temp.LowPart, //
                   .dwHighDateTime = temp.HighPart};
    SYSTEMTIME out;

    if (!FileTimeToSystemTime(&in, &out))
    {
        win32PrintLastError();
    }

    return win32CalendarTime(&out);
}

CalendarTime
localTime(SystemTime sys_time)
{
    // File time conversions according to MS docs:
    // https://docs.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-filetimetolocalfiletime

    ULARGE_INTEGER temp = {.QuadPart = sys_time};
    FILETIME in = {.dwLowDateTime = temp.LowPart, //
                   .dwHighDateTime = temp.HighPart};
    SYSTEMTIME utc, local;

    if (!FileTimeToSystemTime(&in, &utc) || !SystemTimeToTzSpecificLocalTime(NULL, &utc, &local))
    {
        win32PrintLastError();
    }

    return win32CalendarTime(&local);
}

#else
#    error "Time API not implemented for this platform"
#endif
