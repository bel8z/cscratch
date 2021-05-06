#include "string_list.h"

#include "allocator.h"
#include "util.h"

void
slInitAlloc(StringList *sl, cfAllocator *alloc)
{
    CF_ASSERT_NOT_NULL(sl);
    CF_ASSERT_NOT_NULL(alloc);

    sl->alloc = alloc;
    sl->buf = NULL;
    sl->cap = 0;
    sl->len = 0;
    cfListInit(&sl->sentinel);
}

void
slInitBuffer(StringList *sl, void *buffer, usize size)
{
    CF_ASSERT_NOT_NULL(sl);
    CF_ASSERT_NOT_NULL(buffer);

    sl->alloc = NULL;
    sl->buf = buffer;
    sl->cap = size;
    sl->len = 0;
    cfListInit(&sl->sentinel);
}

void
slShutdown(StringList *sl)
{
    slClear(sl);
    if (sl->alloc)
    {
        cfFree(sl->alloc, sl->buf, sl->cap);
        sl->cap = 0;
        sl->buf = NULL;
    }
}

void
slClear(StringList *sl)
{
    cfListInit(&sl->sentinel);
    sl->len = 0;
}

bool
slPush(StringList *sl, char const *str)
{
    usize size = 0;
    do
    {
        size++;
    } while (str[size]);

    usize required = sizeof(StringEntry) + size;
    usize avail = sl->cap - sl->len;

    if (required > avail)
    {
        if (!sl->alloc) return false; // No way to grow

        usize new_cap = cfMax(sl->cap ? sl->cap * 2 : 1, required);
        void *new_buf = cfAlloc(sl->alloc, new_cap);
        if (!new_buf) return false;

        sl->cap = new_cap;
        sl->buf = new_buf;
    }

    char *data = sl->buf + sl->len;

    cfMemCopy(str, data, size);

    sl->len += size;

    StringEntry *entry = sl->buf + sl->len;

    entry->data = data;
    entry->size = size;

    cfListPushTail(&sl->sentinel, &entry->node);

    return true;
}
