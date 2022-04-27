#include "list.h"
#include "error.h"

void
cfListInit(CfList *list)
{
    list->next = list->prev = list;
}

bool
cfListEmpty(CfList const *head)
{
    return (head->next == head);
}

int
cfListIsHead(CfList const *list, CfList const *node)
{
    return (list->next == node);
}

int
cfListIsTail(CfList const *list, CfList const *node)
{
    return (list->prev == node);
}

CfList const *
cfListHead(CfList const *list)
{
    return list->next;
}

CfList const *
cfListTail(CfList const *list)
{
    return list->prev;
}

void
cfListInsert(CfList *node, CfList *prev, CfList *next)
{
    node->prev = prev;
    node->next = next;

    prev->next = node;
    next->prev = node;
}

void
cfListRemove(CfList *node)
{
    node->prev->next = node->next;
    node->next->prev = node->prev;

    node->next = node->prev = node;
}

void
cfListPushHead(CfList *list, CfList *node)
{
    cfListInsert(node, list, list->next);
    CF_ASSERT(list->next == node, "");
}

void
cfListPushTail(CfList *list, CfList *node)
{
    cfListInsert(node, list->prev, list);
    CF_ASSERT(list->prev == node, "");
}

CfList *
cfListPopHead(CfList *list)
{
    CfList *head = list->next;
    cfListRemove(head);
    return head;
}

CfList *
cfListPopTail(CfList *list)
{
    CfList *tail = list->prev;
    cfListRemove(tail);
    return tail;
}
