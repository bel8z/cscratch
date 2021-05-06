#ifndef FOUNDATION_STRING_LIST_H

#include "common.h"

#include "list.h"

typedef struct StringEntry
{
    usize size;
    char *data;
    cfList node;
} StringEntry;

typedef struct StringList
{
    cfAllocator *alloc;

    u8 *buf;
    usize len;
    usize cap;

    cfList sentinel;
} StringList;

void slInitAlloc(StringList *sl, cfAllocator *alloc);
void slInitBuffer(StringList *sl, void *buffer, usize size);
void slShutdown(StringList *sl);

void slClear(StringList *sl);

bool slPush(StringList *sl, char const *str);

#define FOUNDATION_STRING_LIST_H
#endif
