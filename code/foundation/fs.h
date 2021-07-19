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
    FileDialogResult_Ok,
    FileDialogResult_Cancel,
    FileDialogResult_Error,
};

typedef struct FileDialogResult
{
    Str filename;
    U8 code;
} FileDialogResult;

/// File system API
typedef struct CfFileSystem
{
    // *** Directory operations ***

    /// Create an iterator on the given directory contents
    /// Returns false in case of an error
    bool (*dirIterStart)(DirIterator *self, Str dir_path);
    /// Advance the iterator, assigning the entry name to the given string view.
    /// Returns false in case the iteration is terminated (in such a case, the string view is not
    /// valid).
    // NOTE that the string view is valid until the next call to this function (or the iterator is
    // destroyed) so copy its content explicitly if you need to store it.
    bool (*dirIterNext)(DirIterator *self, Str *filename);
    /// Shutdown the iteration
    void (*dirIterEnd)(DirIterator *self);

    // *** File operations ***

    FileContent (*fileRead)(Str filename, CfAllocator alloc);
    bool (*fileCopy)(Str source, Str dest, bool overwrite);
    FileTime (*fileWriteTime)(Str filename);

    // *** File dialogs ***

    // TODO (Matteo): Is this the correct place for file dialogs?

    FileDialogResult (*fileOpenDialog)(Str filename_hint, FileDlgFilter *filters, Usize num_filters,
                                       CfAllocator alloc);

} CfFileSystem;

//------------------------------------------------------------------------------
