#pragma once

#include "foundation/core.h"

typedef struct WorkQueueConfig
{
    void *memory;
    Usize footprint;
    Usize buffer_size;
} WorkQueueConfig;

typedef struct WorkItem
{
    void (*proc)(void *data);
    void *data;
} WorkItem;

typedef struct WorkQueue WorkQueue;

WorkQueueConfig wkConfig(Usize buffer_size);
WorkQueue *wkAllocate(WorkQueueConfig config);

bool wkIsFull(WorkQueue *queue);
bool wkIsEmpty(WorkQueue *queue);

bool wkPush(WorkQueue *queue, WorkItem item);
bool wkPop(WorkQueue *queue, WorkItem *item);
