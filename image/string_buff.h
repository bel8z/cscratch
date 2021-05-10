#ifndef STRING_BUFF_H

#include "foundation/common.h"
#include "foundation/util.h"

enum
{
    StringBuff_MaxCount = 256,
    StringBuff_BuffSize = 256 * 1024,
};

typedef struct StringBuff
{
    u32 index[StringBuff_MaxCount];
    char data[StringBuff_BuffSize];
    u32 count;
} StringBuff;

static inline void
sbClear(StringBuff *sb)
{
    CF_ASSERT_NOT_NULL(sb);
    sb->count = 0;
}

static inline bool
sbPush(StringBuff *sb, char const *str)
{
    CF_ASSERT_NOT_NULL(sb);

    if (sb->count == StringBuff_MaxCount) return false;

    // Compute size of the string, including terminator
    u32 size = 1;
    while (str[size - 1])
    {
        size++;
    }

    u32 offset = sb->index[sb->count];
    u32 avail = StringBuff_BuffSize - offset;
    if (size > avail) return false;

    cfMemCopy(str, sb->data + offset, size);

    sb->index[++sb->count] = offset + size;

    return true;
}

static inline char const *
sbAt(StringBuff *sb, u32 index)
{
    CF_ASSERT_NOT_NULL(sb);
    CF_ASSERT(index < sb->count, "Index out of range");
    return sb->data + sb->index[index];
}

#define STRING_BUFF_H
#endif
