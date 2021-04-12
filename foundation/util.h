// Misc utilities (basically wrapper on common standard library functions as memset and memcpy)
#ifndef UTIL_H

#include "common.h"

#include <string.h>

static inline void
cf_clear_memory(u8 *mem, usize count)
{
    memset(mem, 0, count); // NOLINT
}

static inline void
cf_write_memory(u8 *mem, u8 value, usize count)
{
    memset(mem, value, count); // NOLINT
}

static inline void
cf_copy_memory(u8 const *from, u8 *to, usize count)
{
    memmove_s(to, count, from, count);
}

#define UTIL_H
#endif
