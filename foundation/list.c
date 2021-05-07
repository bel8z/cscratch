#include "list.h"

cfList
cfListCreate(cfList *head)
{
    return (cfList){.next = head, .prev = head};
}

void
cfListInit(cfList *list)
{
    list->next = list->prev = list;
}

bool
cfListEmpty(cfList const *head)
{
    return (head->next == head);
}

int
cfListIsHead(cfList const *list, cfList const *node)
{
    return (list->next == node);
}

int
cfListIsTail(cfList const *list, cfList const *node)
{
    return (list->prev == node);
}

cfList const *
cfListHead(cfList const *list)
{
    return list->next;
}

cfList const *
cfListTail(cfList const *list)
{
    return list->prev;
}

void
cfListInsert(cfList *node, cfList *prev, cfList *next)
{
    node->prev = prev;
    node->next = next;

    prev->next = node;
    next->prev = node;
}

void
cfListRemove(cfList *node)
{
    node->prev->next = node->next;
    node->next->prev = node->prev;

    node->next = node->prev = node;
}

void
cfListPushHead(cfList *list, cfList *node)
{
    cfListInsert(node, list, list->next);
    CF_ASSERT(list->next == node, "");
}

void
cfListPushTail(cfList *list, cfList *node)
{
    cfListInsert(node, list->prev, list);
    CF_ASSERT(list->prev == node, "");
}

cfList *
cfListPopHead(cfList *list)
{
    cfList *head = list->next;
    cfListRemove(head);
    return head;
}

cfList *
cfListPopTail(cfList *list)
{
    cfList *tail = list->prev;
    cfListRemove(tail);
    return tail;
}
