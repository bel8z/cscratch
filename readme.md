# Coding conventions

* Indent 4 spaces, no tabs
* Always use braces, except for single line statements
* Open braces always on a new line (i.e. "Allman" style)
* Types are _PascalCase_
* Functions are _camelCase_
* Variables and fields are _snake_case_ (function pointer fields may use function style)
* Globals are prefixed with g_ and should be used sparingly (the common case is in the file containing the platform layer, which acts as the entry point.)
* Macros are _ALL_CAPS_SNAKE_CASE_, except if they have function semantics (in which case they are typed as functions)
  
## A note about function naming:

Ideally, all functions should be namespaced using a prefix which states the system they belong to (e.g. _appSomething_ for top level application code, _guiButton_, _zipPack_, etc...) or the struct they operate on (which is quite the same concept).

For library public functions, this prefix should start with the library prefix (e.g. _igCreateContext_, _cfArrayPush_).

# Missing features

- [ ] Move OS basics to a library
- [ ] Clean up architecture (platform layer, app layer, foundation and OS libraries)
- [ ] Move memory tracking to foundation library, and let the OS provide only VM and heap functions
- [ ] Strings API
    - [ ] Better nomenclature
    - [ ] Provide utilities for both fixed and dynamic string buffers vs. including a fixed buffer in string builder
- [ ] Paths API
    - [ ] Same as strings API, to which it is dependent
- [ ] Atomics?
- [ ] Networking?


# Timing

## Performance measurement

Win32 offers the QueryPerformanceCounter and QueryPerformanceFrequency functions to get respectively a count in ticks (since an unspecfied time) and the frequency of this counter (number of ticks per second).
The representation is a signed 64 bit integer, even if the results should be always positive.
Typically the resolution is 100ns.

Unix (POSIX?) offers the clock_gettime function: by passing it CLOCK_MONOTONIC as the clock id, it fills a timespec structure with the number of seconds and nanoseconds passed since an unspecified start.
The resolution can be queried by clock_getres(CLOCK_MONOTONIC), but the data structure used enforces
a de-facto maximal resolution in nanoseconds.

The C standard library unsurprisingly follows the UNIX style, and even if it is not much used in this project, employing a data representation that allows easier interoperability with it can be useful for implementing other platform layers.

Maybe UNIX style can be a winner here.

## System time

Win32 offers GetSystemTimePreciseAsFileTime, which returns an unsigned 64 bit integer that represents the amount of units of 100ns since 01/01/1601, with a claimed precision of less than a microsecond.
This format is quite compact, but has an uncommon range (why 1601?).

UNIX offers this information in 2 flavors:
* 



* Windows
    - QueryPerformanceCounter: number of ticks which duration is equal to 1/QueryPerformanceFrequency. Monotonic. Typically units of 100ns.

    - GetSystemTimePreciseAsFileTime: system time with highest possible precision (< 1us), in UTC coordinates. Non monotonic.
    Representation is in units of 100ns since 01/01/1601, expressed as a 64bit value. 

* Unix
    - EPOCH is 01/01/1970
    - gettimeofday: date and time in UTC coordinates, expressed in seconds and microseconds since EPOCH.
    - clock_gettime(CLOCK_REALTIME): system time, seconds and nanoseconds since EPOCH. Non monotonic.
    - clock_gettime(CLOCK_MONOTONIC): monotonic, seconds and nanoseconds since unspecified start.

## Possible duration representations

* Seconds + nanoseconds: good precision and range, ok for interoperation with C11 threading library since it uses UNIX-like representation (timespec).
* Nanoseconds: good precision but range limited to 291 years (signed)
* Win32 performance counter: good precision but range is still limited, and its scope is only perf measurement.
