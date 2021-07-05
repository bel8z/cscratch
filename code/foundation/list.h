#pragma once

#include "core.h"

// Intrusive doubly-linked list

typedef struct cfList cfList;

struct cfList
{
    cfList *prev, *next;
};

#define cfListItem(node, Type, member) (Type *)((U8 *)(node)-offsetof(Type, member))
#define cfListPrevItem(node, Type, member) cfListItem((node)->prev, Type, member)
#define cfListNextItem(node, Type, member) cfListItem((node)->next, Type, member)

inline void
cfListInit(cfList *list)
{
    list->next = list->prev = list;
}

bool cfListEmpty(cfList const *head);

int cfListIsHead(cfList const *list, cfList const *node);
int cfListIsTail(cfList const *list, cfList const *node);

cfList const *cfListHead(cfList const *list);
cfList const *cfListTail(cfList const *list);

void cfListInsert(cfList *node, cfList *prev, cfList *next);
void cfListRemove(cfList *node);

void cfListPushHead(cfList *list, cfList *node);
void cfListPushTail(cfList *list, cfList *node);

cfList *cfListPopHead(cfList *list);
cfList *cfListPopTail(cfList *list);

// TODO (Matteo): Implement iteration safely in case of sentinel nodes
