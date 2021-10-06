#pragma once

#include "core.h"

// TODO (Matteo): Provide async file IO?

/// Iterator over directory content
typedef struct FsIterator
{
    U8 opaque[1280];
} FsIterator;

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

typedef struct File
{
    void *os_handle;
    bool error; // TODO (Matteo): Error detail
    bool eof;
} File;

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

// *** Directory operations ***

/// Create an iterator on the given directory contents
/// Returns false in case of an error
bool fsIteratorStart(FsIterator *self, Str dir_path);
/// Advance the iterator, assigning the entry name to the given string view.
/// Returns false in case the iteration is terminated (in such a case, the string view is not
/// valid).
// NOTE that the string view is valid until the next call to this function (or the iterator is
// destroyed) so copy its content explicitly if you need to store it.
/// File properties are provided optionally (if the props pointer is not null).
bool fsIteratorNext(FsIterator *self, Str *filename, FileProperties *props);
/// Shutdown the iteration
void fsIteratorEnd(FsIterator *self);

// *** File operations ***

bool fileCopy(Str source, Str dest, bool overwrite);
FileProperties fileProperties(Str filename);

File fileOpen(Str filename, FileOpenMode mode);
void fileClose(File *file);

Usize fileSize(File *file);
Usize fileSeek(File *file, FileSeekPos pos, Usize offset);
Usize fileTell(File *file);

Usize fileRead(File *file, U8 *buffer, Usize buffer_size);
Usize fileReadAt(File *file, U8 *buffer, Usize buffer_size, Usize offset);

bool fileWrite(File *file, U8 const *data, Usize data_size);
bool fileWriteAt(File *file, U8 const *data, Usize data_size, Usize offset);
bool fileWriteStr(File *file, Str str);

// NOTE (Matteo): Experimental API

FileContent fileReadContent(Str filename, MemAllocator alloc);

//------------------------------------------------------------------------------
