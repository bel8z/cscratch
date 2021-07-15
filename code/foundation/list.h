#pragma once

/// Foundation linked list utilty
/// This is an API header and as such the only included header must be "core.h"

#include "core.h"

/// Intrusive, circular, doubly-linked list node.
/// In order for a struct to be added to a list, it must contain (at least) a CfList member; this
/// member would have both its next and prev pointer members point to itself when the item is not
/// part of a list.
/// This design allow for simpler code and less memory allocations.
typedef struct CfList CfList;

struct CfList
{
    CfList *prev, *next;
};

#define cfListItem(node, Type, member) (Type *)((U8 *)(node)-offsetof(Type, member))
#define cfListPrevItem(node, Type, member) cfListItem((node)->prev, Type, member)
#define cfListNextItem(node, Type, member) cfListItem((node)->next, Type, member)

inline void
cfListInit(CfList *list)
{
    list->next = list->prev = list;
}

bool cfListEmpty(CfList const *head);

int cfListIsHead(CfList const *list, CfList const *node);
int cfListIsTail(CfList const *list, CfList const *node);

CfList const *cfListHead(CfList const *list);
CfList const *cfListTail(CfList const *list);

void cfListInsert(CfList *node, CfList *prev, CfList *next);
void cfListRemove(CfList *node);

void cfListPushHead(CfList *list, CfList *node);
void cfListPushTail(CfList *list, CfList *node);

CfList *cfListPopHead(CfList *list);
CfList *cfListPopTail(CfList *list);

// TODO (Matteo): Implement iteration safely in case of sentinel nodes
