#ifndef CF_LIST_H

#include "common.h"

// Intrusive doubly-linked list

typedef struct cfList cfList;

struct cfList
{
    cfList *prev, *next;
};

static inline cfList
cf_list_create(cfList *head)
{
    return (cfList){.next = head, .prev = head};
}

static inline void
cf_list_init(cfList *list)
{
    list->next = list->prev = list;
}

#define cf_list_entry(node, type, member) (type *)((u8 const *)(node)-offsetof(type, member))

static inline bool
cf_list_is_empty(cfList const *head)
{
    return (head->next == head);
}

static inline int
cf_list_is_head(cfList const *list, cfList const *node)
{
    return (list->next == node);
}

static inline int
cf_list_is_tail(cfList const *list, cfList const *node)
{
    return (list->prev == node);
}

static inline cfList const *
cf_list_head(cfList const *list)
{
    return list->next;
}

static inline cfList const *
cf_list_tail(cfList const *list)
{
    return list->prev;
}

static inline void
cf_list_insert(cfList *node, cfList *prev, cfList *next)
{
    node->prev = prev;
    node->next = next;

    next->prev = node;
    prev->next = node;
}

static inline void
cf_list_remove(cfList *node)
{
    node->prev->next = node->next;
    node->next->prev = node->prev;

    node->next = node->prev = node;
}

static inline void
cf_list_push_head(cfList *list, cfList *node)
{
    cf_list_insert(node, list, list->next);
    assert(list->next == node);
}

static inline void
cf_list_push_tail(cfList *list, cfList *node)
{
    cf_list_insert(node, list->prev, list);
    assert(list->prev == node);
}

static inline cfList *
cf_list_pop_head(cfList *list)
{
    cfList *head = list->next;
    cf_list_remove(head);
    return head;
}

static inline cfList *
cf_list_pop_tail(cfList *list)
{
    cfList *tail = list->prev;
    cf_list_remove(tail);
    return tail;
}

static inline bool
cf_list_iter_next(cfList const **cursor)
{
    *cursor = (*cursor)->next;
    return (*cursor != (*cursor)->next);
}

static inline bool
cf_list_iter_prev(cfList const **cursor)
{
    *cursor = (*cursor)->prev;
    return (*cursor != (*cursor)->prev);
}

#define CF_LIST_H
#endif
