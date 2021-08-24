#pragma once

#include "foundation/atom.h"
#include "foundation/core.h"
#include "foundation/threading.h"

typedef struct WorkItem
{
    void (*proc)(void *data);
    void *data;
} WorkItem;

typedef struct WorkQueue
{
    AtomUsize read, write;
    Usize size;
    WorkItem *buffer;
} WorkQueue;

bool
wkPush(WorkQueue *queue, WorkItem item)
{
    Usize write = atomRead(&queue->write);
    if (write < atomRead(&queue->read) + queue->size)
    {
        atomAcquireFence();
        queue->buffer[write] = item;
        atomAcquireFence();
        atomWrite(&queue->write, write + 1);
    }
}
