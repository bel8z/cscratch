#pragma once

#include "foundation/core.h"

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

#define TASK_QUEUE_PROC(name) void name(void *data)
typedef TASK_QUEUE_PROC((*TaskProc));

typedef Usize TaskID;

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
bool taskStartProcessing(TaskQueue *queue);
/// Stop processing of queued tasks, optionally flushing them.
bool taskStopProcessing(TaskQueue *queue, bool flush);

/// Enqueue a task for processing
Usize taskEnqueue(TaskQueue *queue, TaskProc proc, void *data);

/// Assist the task queue by performing a pending task on the current thread, if present
bool taskTryWork(TaskQueue *queue);
