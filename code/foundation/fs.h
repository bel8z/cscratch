#pragma once

#include "core.h"

// TODO (Matteo): Provide async file IO?

typedef U32 FileAttributes;
enum FileAttributes_
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

// *** Directory operations ***

/// Iterator over directory content
typedef struct FsIterator FsIterator;

#define FS_ITERATOR_NEXT(name) bool name(FsIterator *self, Str *filename, FileProperties *props)

struct FsIterator
{
    U8 opaque[1280];
    /// Advance the iterator, assigning the entry name to the given string view.
    /// Returns false in case the iteration is terminated (in such a case, the string view is not
    /// valid).
    // NOTE that the string view is valid until the next call to this function (or the iterator is
    // destroyed) so copy its content explicitly if you need to store it.
    /// File properties are provided optionally (if the props pointer is not null).
    FS_ITERATOR_NEXT((*next));
};

/// Create an iterator on the given directory contents
/// Returns false in case of an error
bool fsIteratorStart(FsIterator *self, Str dir_path);
/// Shutdown the iteration
void fsIteratorEnd(FsIterator *self);

// *** File operations ***

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

typedef struct File File;

#define FILE_SIZE(name) Usize name(File *file)
#define FILE_SEEK(name) Usize name(File *file, FileSeekPos pos, Usize offset)
#define FILE_TELL(name) Usize name(File *file)
#define FILE_READ(name) Usize name(File *file, U8 *buffer, Usize buffer_size)
#define FILE_READ_AT(name) Usize name(File *file, U8 *buffer, Usize buffer_size, Usize offset)
#define FILE_WRITE(name) bool name(File *file, U8 const *data, Usize data_size)
#define FILE_WRITE_AT(name) bool name(File *file, U8 const *data, Usize data_size, Usize offset)

struct File
{
    void *os_handle;
    bool error; // TODO (Matteo): Error detail
    bool eof;

    FILE_SIZE((*size));
    FILE_SEEK((*seek));
    FILE_TELL((*tell));
    FILE_READ((*read));
    FILE_READ_AT((*readAt));
    FILE_WRITE((*write));
    FILE_WRITE_AT((*writeAt));
};

typedef struct FileContent
{
    U8 *data;
    Usize size;
} FileContent;

bool fileCopy(Str source, Str dest, bool overwrite);
FileProperties fileProperties(Str filename);

File fileOpen(Str filename, FileOpenMode mode);
void fileClose(File *file);

bool fileWriteStr(File *file, Str str);

// NOTE (Matteo): Experimental API

FileContent fileReadContent(Str filename, MemAllocator alloc);

//------------------------------------------------------------------------------
