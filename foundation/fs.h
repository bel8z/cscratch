#ifndef FOUNDATION_FS_H

#include "common.h"

// Iterates over directory entries
typedef struct DirIter DirIter;

typedef struct FileFilter
{
    // Display name of the filter
    char *name;
    // Supported extensions as a series of null terminated strings (ends with 2 null terminators)
    char *extensions;
} FileFilter;

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

    char *(*open_file_dlg)(char const *filename_hint, FileFilter *filters, usize num_filters,
                           cfAllocator *alloc, u32 *out_size);

} cfFileSystem;

//------------------------------------------------------------------------------

#define FOUNDATION_FS_H
#endif
