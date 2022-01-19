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

CF_INLINE_API void cfListInit(CfList *list);

CF_INLINE_API bool cfListEmpty(CfList const *head);

CF_INLINE_API int cfListIsHead(CfList const *list, CfList const *node);
CF_INLINE_API int cfListIsTail(CfList const *list, CfList const *node);

CF_INLINE_API CfList const *cfListHead(CfList const *list);
CF_INLINE_API CfList const *cfListTail(CfList const *list);

CF_INLINE_API void cfListInsert(CfList *node, CfList *prev, CfList *next);
CF_INLINE_API void cfListRemove(CfList *node);

CF_INLINE_API void cfListPushHead(CfList *list, CfList *node);
CF_INLINE_API void cfListPushTail(CfList *list, CfList *node);

CF_INLINE_API CfList *cfListPopHead(CfList *list);
CF_INLINE_API CfList *cfListPopTail(CfList *list);

// TODO (Matteo): Implement iteration safely in case of sentinel nodes
