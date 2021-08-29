#pragma once

#include "core.h"

// TODO (Matteo): Provide async file IO?

/// Iterator over directory content
typedef struct DirIterator
{
    U8 opaque[1280];
} DirIterator;

typedef U32 FileAttributes;
enum FileAttributes_
{
    FileAttributes_Directory = 1,
    FileAttributes_Symlink = 2,
};

typedef U8 FileOpenMode;
enum FileOpenMode_
{
    FileOpenMode_Read = 1,
    FileOpenMode_Write = 2,
    FileOpenMode_RW = FileOpenMode_Read | FileOpenMode_Write,
    FileOpenMode_Append = FileOpenMode_Write | 4,
};

typedef U8 FileSeekPos;
enum FileSeekPos_
{
    FileSeekPos_Begin = 0,
    FileSeekPos_Current,
    FileSeekPos_End,
};

typedef U32 FileStreamFlags;
enum FileStreamFlags_
{
    FileStreamFlags_Error,
    FileStreamFlags_Eof,
};

typedef struct FileStream
{
    void *os_handle;
    FileStreamFlags flags;
} FileStream;

typedef struct FileContent
{
    U8 *data;
    Usize size;
} FileContent;

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

    bool (*fileCopy)(Str source, Str dest, bool overwrite);
    FileProperties (*fileProperties)(Str filename);

    FileStream (*fileStreamOpen)(Str filename, FileOpenMode mode);
    void (*fileStreamClose)(FileStream *file);

    Usize (*fileStreamSize)(FileStream *file);
    Usize (*fileStreamSeek)(FileStream *file, FileSeekPos pos, Usize offset);
    Usize (*fileStreamTell)(FileStream *file);

    Usize (*fileStreamRead)(FileStream *file, U8 *buffer, Usize buffer_size);
    Usize (*fileStreamReadAt)(FileStream *file, U8 *buffer, Usize buffer_size, Usize offset);

    bool (*fileStreamWrite)(FileStream *file, U8 *data, Usize data_size);
    bool (*fileStreamWriteAt)(FileStream *file, U8 *data, Usize data_size, Usize offset);

} CfFileSystem;

FileContent fileReadContent(CfFileSystem *fs, Str filename, MemAllocator alloc);

//------------------------------------------------------------------------------
