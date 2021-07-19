#pragma once

#include "core.h"

/// Iterator over directory content
typedef struct DirIterator
{
    U8 opaque[1024];
} DirIterator;

typedef struct FileContent
{
    U8 *data;
    Usize size;
} FileContent;

typedef U64 FileTime;

typedef struct FileDlgFilter
{
    // Display name of the filter
    Cstr name;
    // Supported extensions
    Cstr *extensions;
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
    Str filename;
    U8 code;
} FileDlgResult;

/// File system API
typedef struct CfFileSystem
{
    // *** Directory operations ***

    /// Create an iterator on the given directory contents (return NULL in case of failure)
    bool (*dirIterStart)(DirIterator *self, Str dir_path);
    /// Advance the iterator and return the filename of the current entry, or NULL if the iteration
    /// is complete; NOTE that the current pointer is valid until the next call to this function (or
    /// the iterator is destroyed)
    bool (*dirIterNext)(DirIterator *self, Str *filename);
    /// Shutdown the iteration
    void (*dirIterEnd)(DirIterator *self);

    // *** File operations ***

    FileContent (*fileRead)(Str filename, CfAllocator alloc);
    bool (*fileCopy)(Str source, Str dest, bool overwrite);
    FileTime (*fileWriteTime)(Str filename);

    // *** File dialogs ***

    // TODO (Matteo): Is this the correct place for file dialogs?

    FileDlgResult (*open_file_dlg)(Str filename_hint, FileDlgFilter *filters, Usize num_filters,
                                   CfAllocator alloc);

} CfFileSystem;

//------------------------------------------------------------------------------
