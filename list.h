#ifndef LIST_H

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// Intrusive doubly-linked list

typedef struct List List;

struct List
{
    List *prev, *next;
};

static inline List
list_create(List *head)
{
    return (List){.next = head, .prev = head};
}

static inline void
list_init(List *list)
{
    list->next = list->prev = list;
}

#define list_entry(node, type, member)                                         \
    (type *)((uint8_t const *)(node)-offsetof(type, member))

static inline bool
list_is_empty(List const *head)
{
    return (head->next == head);
}

static inline int
list_is_head(List const *list, List const *node)
{
    return (list->next == node);
}

static inline int
list_is_tail(List const *list, List const *node)
{
    return (list->prev == node);
}

static inline List const *
list_head(List const *list)
{
    return list->next;
}

static inline List const *
list_tail(List const *list)
{
    return list->prev;
}

static inline void
list_insert(List *node, List *prev, List *next)
{
    node->prev = prev;
    node->next = next;

    next->prev = node;
    prev->next = node;
}

static inline void
list_remove(List *node)
{
    node->prev->next = node->next;
    node->next->prev = node->prev;

    node->next = node->prev = node;
}

static inline void
list_push_head(List *list, List *node)
{
    list_insert(node, list, list->next);
    assert(list->next == node);
}

static inline void
list_push_tail(List *list, List *node)
{
    list_insert(node, list->prev, list);
    assert(list->prev == node);
}

static inline List *
list_pop_head(List *list)
{
    List *head = list->next;
    list_remove(head);
    return head;
}

static inline List *
list_pop_tail(List *list)
{
    List *tail = list->prev;
    list_remove(tail);
    return tail;
}

static inline bool
list_iter_next(List const **cursor)
{
    *cursor = (*cursor)->next;
    return (*cursor != (*cursor)->next);
}

static inline bool
list_iter_prev(List const **cursor)
{
    *cursor = (*cursor)->prev;
    return (*cursor != (*cursor)->prev);
}

#define LIST_H
#endif
