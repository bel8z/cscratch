#pragma once

#include "common.h"

#include "list.h"

typedef struct StringEntry
{
    Usize size;
    char *data;
    cfList node;
} StringEntry;

typedef struct StringList
{
    // Number of stored strings
    Usize count;

    // Internal list for iteration
    cfList list;

    // Internal memory management
    // cfAllocator *alloc;
    Usize cap;
    Usize len;
    U8 *buf;
} StringList;

void slInitBuffer(StringList *sl, U8 *buffer, Usize size);
// void slInitAlloc(StringList *sl, cfAllocator *alloc);
void slShutdown(StringList *sl);

void slClear(StringList *sl);
bool slPush(StringList *sl, char const *str);
bool slPop(StringList *sl);

StringEntry *slFirst(StringList const *sl);
StringEntry *slLast(StringList const *sl);

bool slIterNext(StringList const *sl, StringEntry **entry);
bool slIterPrev(StringList const *sl, StringEntry **entry);
