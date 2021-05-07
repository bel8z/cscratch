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
    // Number of stored strings
    usize count;

    // Internal list for iteration
    cfList list;

    // Internal memory management
    cfAllocator *alloc;
    usize cap;
    usize len;
    u8 *buf;
} StringList;

void slInitBuffer(StringList *sl, u8 *buffer, usize size);
void slInitAlloc(StringList *sl, cfAllocator *alloc);
void slShutdown(StringList *sl);

void slClear(StringList *sl);
bool slPush(StringList *sl, char const *str);
bool slPop(StringList *sl);

bool slIterNext(StringList const *buff, StringEntry **entry);

#define FOUNDATION_STRING_LIST_H
#endif
