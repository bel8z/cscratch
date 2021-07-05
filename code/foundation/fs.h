#pragma once

#include "common.h"

/// Iterator over directory content
typedef struct DirIter DirIter;

typedef struct FileContent
{
    U8 *data;
    Usize size;
} FileContent;

typedef U64 FileTime;

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

/// File system API
typedef struct cfFileSystem
{
    // *** Directory operations ***

    /// Create an iterator on the given directory contents (return NULL in case of failure)
    DirIter *(*dirIterStart)(char const *dir, cfAllocator alloc);
    /// Advance the iterator and return the filename of the current entry, or NULL if the iteration
    /// is complete; NOTE that the current pointer is valid until the next call to this function (or
    /// the iterator is destroyed)
    char const *(*dirIterNext)(DirIter *iter);
    /// Shutdown the iteration
    void (*dirIterClose)(DirIter *iter);

    // *** File operations ***

    FileContent (*fileRead)(char const *filename, cfAllocator alloc);
    bool (*fileCopy)(char const *source, char const *dest, bool overwrite);
    FileTime (*fileWrite)(char const *filename);

    // *** File dialogs ***

    // TODO (Matteo): Is this the correct place for file dialogs?

    FileDlgResult (*open_file_dlg)(char const *filename_hint, FileDlgFilter *filters,
                                   Usize num_filters, cfAllocator alloc);

} cfFileSystem;

//------------------------------------------------------------------------------
