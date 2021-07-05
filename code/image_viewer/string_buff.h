#include "foundation/core.h"
#include "foundation/strings.h"

enum
{
    StringBuff_MaxCount = 256,
    StringBuff_BuffSize = StringBuff_MaxCount * 1024,
};

// NOTE (Matteo): Zero initialization required

typedef struct StringBuff
{
    U32 index[StringBuff_MaxCount];
    char data[StringBuff_BuffSize];
    U32 count;
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
    Usize size = strSize(str);

    U32 offset = sb->index[sb->count];
    U32 avail = StringBuff_BuffSize - offset;
    if (size > avail) return false;

    cfMemCopy(str, sb->data + offset, size);

    sb->index[++sb->count] = offset + (U32)size;

    return true;
}

static inline char const *
sbAt(StringBuff *sb, U32 index)
{
    CF_ASSERT_NOT_NULL(sb);
    CF_ASSERT(index < sb->count, "Index out of range");
    return sb->data + sb->index[index];
}
