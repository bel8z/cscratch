#pragma once

/// Foundation linked list utilty
/// This is an API header and as such the only included header must be "core.h"

#include "core.h"

/// Intrusive, circular, doubly-linked list node.
/// In order for a struct to be added to a list, it must contain (at least) a cfList member; this
/// member would have both its next and prev pointer members point to itself when the item is not
/// part of a list.
/// This design allow for simpler code and less memory allocations.
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
