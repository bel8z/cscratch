#ifndef CF_LIST_H

#include "common.h"

// Intrusive doubly-linked list

typedef struct cfList cfList;

struct cfList
{
    cfList *prev, *next;
};

#define cfListItem(node, Type, member) (Type *)((u8 *)(node)-offsetof(Type, member));
#define cfListPrevItem(node, type, member) cfListItem((node)->prev, type, member)
#define cfListNextItem(node, type, member) cfListItem((node)->next, type, member)

cfList cfListCreate(cfList *head);
void cfListInit(cfList *list);

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

#define CF_LIST_H
#endif
