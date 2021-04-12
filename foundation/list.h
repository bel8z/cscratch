#ifndef CF_LIST_H

#include "common.h"

// Intrusive doubly-linked list

typedef struct CfList CfList;

struct CfList
{
    CfList *prev, *next;
};

static inline CfList
cf_list_create(CfList *head)
{
    return (CfList){.next = head, .prev = head};
}

static inline void
cf_list_init(CfList *list)
{
    list->next = list->prev = list;
}

#define cf_list_entry(node, type, member) (type *)((u8 const *)(node)-offsetof(type, member))

static inline bool
cf_list_is_empty(CfList const *head)
{
    return (head->next == head);
}

static inline int
cf_list_is_head(CfList const *list, CfList const *node)
{
    return (list->next == node);
}

static inline int
cf_list_is_tail(CfList const *list, CfList const *node)
{
    return (list->prev == node);
}

static inline CfList const *
cf_list_head(CfList const *list)
{
    return list->next;
}

static inline CfList const *
cf_list_tail(CfList const *list)
{
    return list->prev;
}

static inline void
cf_list_insert(CfList *node, CfList *prev, CfList *next)
{
    node->prev = prev;
    node->next = next;

    next->prev = node;
    prev->next = node;
}

static inline void
cf_list_remove(CfList *node)
{
    node->prev->next = node->next;
    node->next->prev = node->prev;

    node->next = node->prev = node;
}

static inline void
cf_list_push_head(CfList *list, CfList *node)
{
    cf_list_insert(node, list, list->next);
    assert(list->next == node);
}

static inline void
cf_list_push_tail(CfList *list, CfList *node)
{
    cf_list_insert(node, list->prev, list);
    assert(list->prev == node);
}

static inline CfList *
cf_list_pop_head(CfList *list)
{
    CfList *head = list->next;
    cf_list_remove(head);
    return head;
}

static inline CfList *
cf_list_pop_tail(CfList *list)
{
    CfList *tail = list->prev;
    cf_list_remove(tail);
    return tail;
}

static inline bool
cf_list_iter_next(CfList const **cursor)
{
    *cursor = (*cursor)->next;
    return (*cursor != (*cursor)->next);
}

static inline bool
cf_list_iter_prev(CfList const **cursor)
{
    *cursor = (*cursor)->prev;
    return (*cursor != (*cursor)->prev);
}

#define CF_LIST_H
#endif
