#pragma once

#include "foundation/core.h"

#define WORK_QUEUE_PROC(name) void name(void *data)

typedef WORK_QUEUE_PROC((*TaskProc));

/// Task queue configuration struct
typedef struct TaskQueueConfig
{
    /// [In] Size of the internal FIFO buffer
    Usize buffer_size;
    /// [In] Number of worker threads that service the queue
    Usize num_workers;
    /// [Out] Memory footprint of the configured queue
    Usize footprint;
} TaskQueueConfig;

/// Opaque type representing the task queue
typedef struct TaskQueue TaskQueue;

/// Configure the task queue creation
bool taskConfig(TaskQueueConfig *config);

/// Initializes the task queue in the given block of memory.
/// The size of the block is specified by the 'footprint' member of the config struct.
/// The queue is idle upon initialization, and processing must be explicitly started.
TaskQueue *taskInit(TaskQueueConfig *config, void *memory);

/// Shutdowns the task queue.
void taskShutdown(TaskQueue *queue);

/// Start processing of queued tasks
void taskStartProcessing(TaskQueue *queue);
/// Stop processing of queued tasks, optionally flushing them.
void taskStopProcessing(TaskQueue *queue, bool flush);

/// Enqueue a task for processing
bool taskEnqueue(TaskQueue *queue, TaskProc proc, void *data);

/// Assist the task queue by performing a pending task on the current thread, if present
bool taskTryWork(TaskQueue *queue);
