#pragma once

#include "foundation/core.h"

#define WORK_QUEUE_PROC(name) void name(void *data)

typedef WORK_QUEUE_PROC((*WorkQueueProc));

typedef struct WorkQueueConfig
{
    // In
    Usize buffer_size;
    Usize num_workers;
    // Out
    Usize footprint;
} WorkQueueConfig;

typedef struct WorkQueue WorkQueue;

bool worqConfig(WorkQueueConfig *config);
WorkQueue *worqInit(WorkQueueConfig *config, void *memory);
void worqShutdown(WorkQueue *queue);

void worqStartProcessing(WorkQueue *queue);
void worqStopProcessing(WorkQueue *queue, bool flush);
bool worqEnqueue(WorkQueue *queue, WorkQueueProc proc, void *data);
