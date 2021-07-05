#include "string_list.h"

#include "core.h"
#include "strings.h"
#include "util.h"

void
slInitBuffer(StringList *sl, U8 *buffer, Usize size)
{
    // sl->alloc = NULL;
    sl->cap = size;
    sl->len = 0;
    sl->buf = buffer;

    sl->count = 0;

    cfListInit(&sl->list);
}

// void
// slInitAlloc(StringList *sl, cfAllocator *alloc)
// {
//     sl->alloc = alloc;
//     sl->cap = 0;
//     sl->len = 0;
//     sl->buf = NULL;

//     sl->count = 0;

//     cfListInit(&sl->list);
// }

void
slShutdown(StringList *sl)
{
    slClear(sl);
    // if (sl->alloc)
    // {
    //     cfFree(sl->alloc, sl->buf, sl->cap);
    // }
}

void
slClear(StringList *sl)
{
    sl->count = 0;
    sl->len = 0;
    cfListInit(&sl->list);
}

bool
slPush(StringList *sl, char const *str)
{
    // Compute size of the string, including terminator
    Usize size = strSize(str);

    StringEntry *entry = NULL;
    Usize avail = sl->cap - sl->len;
    Usize required = sizeof(*entry) + size;

    if (required > avail)
    {
        return false;

        // if (!sl->alloc) return false;

        // Usize new_cap = cfMax(required, sl->cap ? sl->cap * 2 : 1);
        // U8 *new_buf = cfRealloc(sl->alloc, sl->buf, sl->cap, new_cap);
        // if (!new_buf) return false;

        // sl->cap = new_cap;
        // sl->buf = new_buf;
    }

    entry = (StringEntry *)(sl->buf + sl->len);
    entry->data = (char *)(entry + 1);
    entry->size = size;

    cfListPushTail(&sl->list, &entry->node);

    CF_ASSERT(sl->list.prev == &entry->node, "");
    CF_ASSERT(entry->node.next == &sl->list, "");

    cfMemCopy(str, entry->data, size);

    sl->len += required;
    sl->count++;

    return true;
}

bool
slPop(StringList *sl)
{
    if (!sl->len) return false;

    cfList *tail = cfListPopTail(&sl->list);

    CF_ASSERT(tail != &sl->list, "Pop of list sentinel");

    StringEntry *entry = cfListItem(tail, StringEntry, node);

    Usize block_size = sizeof(*entry) + entry->size;

    CF_ASSERT(block_size <= sl->len, "Removing more than available");

    sl->count--;
    sl->len -= block_size;

    return true;
}

StringEntry *
slFirst(StringList const *sl)
{
    return sl->len ? cfListItem(sl->list.next, StringEntry, node) : NULL;
}

StringEntry *
slLast(StringList const *sl)
{
    return sl->len ? cfListItem(sl->list.prev, StringEntry, node) : NULL;
}

static bool
sl__IterMove(StringList const *sl, StringEntry **entry, bool forward)
{
    CF_ASSERT_NOT_NULL(sl);
    CF_ASSERT_NOT_NULL(entry);

    cfList const *list = &sl->list;
    cfList const *curr = (*entry) ? &(*entry)->node : list;
    cfList const *next = forward ? curr->next : curr->prev;

    if (next == list) return false;

    *entry = cfListItem(next, StringEntry, node);

    return true;
}

bool
slIterNext(StringList const *sl, StringEntry **entry)
{
    return sl__IterMove(sl, entry, true);
}

bool
slIterPrev(StringList const *sl, StringEntry **entry)
{
    return sl__IterMove(sl, entry, false);
}
