#pragma once

#include "common.h"

// Iterates over directory entries
typedef struct DirIter DirIter;

typedef struct FileDlgFilter
{
    // Display name of the filter
    char const *name;
    // Supported extensions
    char const **extensions;
    Usize num_extensions;
} FileDlgFilter;

enum
{
    FileDlgResult_Ok,
    FileDlgResult_Cancel,
    FileDlgResult_Error,
};

typedef struct FileDlgResult
{
    Usize filename_size;
    char *filename;
    U8 code;
} FileDlgResult;

typedef struct FileContent
{
    U8 *data;
    Usize size;
} FileContent;

typedef struct cfFileSystem
{
    // Create an iterator on the given directory contents (return NULL in case of failure)
    DirIter *(*dir_iter_start)(char const *dir, cfAllocator *alloc);
    // Advance the iterator and return the filename of the current entry, or NULL
    // if the iteration is complete; NOTE that the current pointer is valid
    // until the next call to this function (or the iterator is destroyed)
    char const *(*dir_iter_next)(DirIter *iter);
    // Shutdown the iteration
    void (*dir_iter_close)(DirIter *iter);

    FileDlgResult (*open_file_dlg)(char const *filename_hint, FileDlgFilter *filters,
                                   Usize num_filters, cfAllocator *alloc);

    FileContent (*read_file)(char const *filename, cfAllocator *alloc);

} cfFileSystem;

//------------------------------------------------------------------------------
