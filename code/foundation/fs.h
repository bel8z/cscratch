#pragma once

#include "core.h"

/// Iterator over directory content
typedef struct DirIterator
{
    U8 opaque[1280];
} DirIterator;

typedef struct FileContent
{
    U8 *data;
    Usize size;
} FileContent;

typedef U32 FileAttributes;

enum
{
    FileAttributes_Directory = 1,
    FileAttributes_Symlink = 2,
};

typedef struct FileProperties
{
    SystemTime last_write;
    FileAttributes attributes;
    bool exists;
} FileProperties;

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
    /// File properties are provided optionally (if the props pointer is not null).
    bool (*dirIterNext)(DirIterator *self, Str *filename, FileProperties *props);
    /// Shutdown the iteration
    void (*dirIterEnd)(DirIterator *self);

    // *** File operations ***

    FileContent (*fileRead)(Str filename, MemAllocator alloc);
    bool (*fileCopy)(Str source, Str dest, bool overwrite);
    FileProperties (*fileProperties)(Str filename);

} CfFileSystem;

//------------------------------------------------------------------------------
