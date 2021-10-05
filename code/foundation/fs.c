#include "fs.h"
#include "memory.h"

//-----------------------//
//   Common operations   //
//-----------------------//

FileContent
fileReadContent(Str filename, MemAllocator alloc)
{
    FileContent result = {0};

    FileStream file = fileStreamOpen(filename, FileOpenMode_Read);

    if (!(file.flags & FileStreamFlags_Error))
    {
        Usize file_size = fileStreamSize(&file);
        Usize read_size = file_size;

        result.data = memAlloc(alloc, read_size);

        if (result.data && fileStreamRead(&file, result.data, read_size) == read_size)
        {
            result.size = read_size;
        }
        else
        {
            memFree(alloc, result.data, read_size);
            result.data = NULL;
        }

        fileStreamClose(&file);
    }

    return result;
}

//--------------------------------//
//   OS-specific implementation   //
//--------------------------------//

#ifdef CF_OS_WIN32

#    include "win32.h"

typedef struct Win32DirIterator
{
    HANDLE finder;
    WIN32_FIND_DATAW data;
    Char8 buffer[sizeof(DirIterator) - sizeof(HANDLE) - sizeof(WIN32_FIND_DATAW)];
} Win32DirIterator;

static void
win32ReadProperties(WIN32_FIND_DATAW *in, FileProperties *out)
{
    out->exists = true;

    FILETIME write_time = in->ftLastWriteTime;
    out->last_write = ((U64)write_time.dwHighDateTime << 32) | write_time.dwLowDateTime;

    if (in->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
    {
        out->attributes |= FileAttributes_Directory;
    }

    if (in->dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT)
    {
        out->attributes |= FileAttributes_Symlink;
    }
}

static OVERLAPPED
win32FileOffset(Usize offset)
{
    ULARGE_INTEGER temp_offset = {.QuadPart = offset};
    OVERLAPPED overlapped = {.Offset = temp_offset.LowPart, .OffsetHigh = temp_offset.HighPart};
    return overlapped;
}

bool
dirIterStart(DirIterator *self, Str dir_path)
{
    CF_ASSERT_NOT_NULL(self);

    Win32DirIterator *iter = (Win32DirIterator *)self->opaque;
    Char16 buffer[1024];

    // Encode path to UTF16
    Usize length = win32Utf8To16(dir_path, buffer, CF_ARRAY_SIZE(buffer));

    if (length == USIZE_MAX || length >= CF_ARRAY_SIZE(buffer) - 2)
    {
        CF_ASSERT(false, "Encoding error or overflow");
        return false;
    }

    // Append a wildcard (D:)
    buffer[length] = L'*';
    buffer[length + 1] = 0;

    // Start iteration
    iter->finder = FindFirstFileW(buffer, &iter->data);
    if (iter->finder == INVALID_HANDLE_VALUE) return false;

    // Skip to ".." so that the "advance" method can call FindNextFileW directly
    if (!wcscmp(iter->data.cFileName, L".") && !FindNextFileW(iter->finder, &iter->data))
    {
        FindClose(iter->finder);
        iter->finder = INVALID_HANDLE_VALUE;
        return false;
    }

    return true;
}

bool
dirIterNext(DirIterator *self, Str *filename, FileProperties *props)
{
    CF_ASSERT_NOT_NULL(self);

    Win32DirIterator *iter = (Win32DirIterator *)self->opaque;

    if (!FindNextFileW(iter->finder, &iter->data)) return false;

    Usize size = win32Utf16To8(str16FromCstr(iter->data.cFileName), iter->buffer,
                               CF_ARRAY_SIZE(iter->buffer));

    // NOTE (Matteo): Truncation is considered an error
    // TODO (Matteo): Maybe require a bigger buffer?
    if (size == USIZE_MAX || size == CF_ARRAY_SIZE(iter->buffer)) return false;

    CF_ASSERT(size > 0, "Which filename can have a size of 0???");

    filename->buf = iter->buffer;
    filename->len = (Usize)(size);

    if (props) win32ReadProperties(&iter->data, props);

    return true;
}

void
dirIterEnd(DirIterator *self)
{
    CF_ASSERT_NOT_NULL(self);

    Win32DirIterator *iter = (Win32DirIterator *)self->opaque;
    FindClose(iter->finder);
}

bool
fileCopy(Str source, Str dest, bool overwrite)
{
    Char16 ws[MAX_PATH] = {0};
    Char16 wd[MAX_PATH] = {0};
    win32Utf8To16(source, ws, CF_ARRAY_SIZE(ws));
    win32Utf8To16(dest, wd, CF_ARRAY_SIZE(wd));

    if (CopyFileW(ws, wd, !overwrite)) return true;

    win32PrintLastError();
    return false;
}

FileProperties
fileProperties(Str filename)
{
    FileProperties props = {0};

    Char16 wide_name[MAX_PATH] = {0};
    win32Utf8To16(filename, wide_name, CF_ARRAY_SIZE(wide_name));

    WIN32_FIND_DATAW data = {0};
    HANDLE find_handle = FindFirstFileW(wide_name, &data);

    if (find_handle != INVALID_HANDLE_VALUE)
    {
        win32ReadProperties(&data, &props);
        FindClose(find_handle);
    }

    return props;
}

FileStream
fileStreamOpen(Str filename, FileOpenMode mode)
{
    FileStream result = {0};

    if (mode == 0)
    {
        result.flags |= FileStreamFlags_Error;
    }
    else
    {
        DWORD creation = 0;
        DWORD access = 0;

        if (mode & FileOpenMode_Write)
        {
            creation = (mode & FileOpenMode_Append) ? OPEN_ALWAYS : CREATE_NEW;
            access |= GENERIC_WRITE;
        }
        else
        {
            CF_ASSERT(mode & FileOpenMode_Read, "Invalid file open mode");
            creation = OPEN_EXISTING;
            access |= GENERIC_READ;
        }

        Char16 buffer[1024] = {0};
        win32Utf8To16(filename, buffer, CF_ARRAY_SIZE(buffer));

        result.os_handle = CreateFileW(buffer, access, FILE_SHARE_READ, NULL, creation,
                                       FILE_ATTRIBUTE_NORMAL, NULL);

        if (!result.os_handle)
        {
            result.flags |= FileStreamFlags_Error;
            win32PrintLastError();
        }
        else if (mode & FileOpenMode_Append)
        {
            if (!SetFilePointer(result.os_handle, 0, NULL, FILE_END))
            {
                result.flags |= FileStreamFlags_Error;
                win32PrintLastError();
            }
        }
    }

    return result;
}

void
fileStreamClose(FileStream *file)
{
    CloseHandle(file->os_handle);
}

Usize
fileStreamRead(FileStream *file, U8 *buffer, Usize buffer_size)
{
    if (file->flags & FileStreamFlags_Error) return USIZE_MAX;

    DWORD read_bytes;

    if (!ReadFile(file->os_handle, buffer, (DWORD)buffer_size, &read_bytes, NULL))
    {
        file->flags |= FileStreamFlags_Error;
        win32PrintLastError();
        return USIZE_MAX;
    }
    else if (read_bytes < buffer_size)
    {
        file->flags |= FileStreamFlags_Eof;
    }

    return read_bytes;
}

Usize
fileStreamReadAt(FileStream *file, U8 *buffer, Usize buffer_size, Usize offset)
{
    if (file->flags & FileStreamFlags_Error) return USIZE_MAX;

    OVERLAPPED overlapped = win32FileOffset(offset);
    DWORD read_bytes;

    if (!ReadFile(file->os_handle, buffer, (DWORD)buffer_size, &read_bytes, &overlapped))
    {
        file->flags |= FileStreamFlags_Error;
        win32PrintLastError();
        return USIZE_MAX;
    }

    return read_bytes;
}

bool
fileStreamWrite(FileStream *file, U8 *data, Usize data_size)
{
    if (file->flags & FileStreamFlags_Error) return false;

    DWORD written_bytes;

    if (!WriteFile(file->os_handle, data, (DWORD)data_size, &written_bytes, NULL))
    {
        file->flags |= FileStreamFlags_Error;
        win32PrintLastError();
        return false;
    }

    CF_ASSERT(written_bytes == (DWORD)data_size, "Incorrect number of bytes written");

    return true;
}

bool
fileStreamWriteAt(FileStream *file, U8 *data, Usize data_size, Usize offset)
{
    if (file->flags & FileStreamFlags_Error) return false;

    OVERLAPPED overlapped = win32FileOffset(offset);
    DWORD written_bytes;

    if (!WriteFile(file->os_handle, data, (DWORD)data_size, &written_bytes, &overlapped))
    {
        file->flags |= FileStreamFlags_Error;
        win32PrintLastError();
        return false;
    }

    CF_ASSERT(written_bytes == (DWORD)data_size, "Incorrect number of bytes written");

    return true;
}

Usize
fileStreamSeek(FileStream *file, FileSeekPos pos, Usize offset)
{
    LARGE_INTEGER temp = {.QuadPart = (LONGLONG)offset};
    LARGE_INTEGER dest = {0};

    if (!SetFilePointerEx(file->os_handle, temp, &dest, pos))
    {
        file->flags |= FileStreamFlags_Error;
        win32PrintLastError();
    };

    return (Usize)dest.QuadPart;
}

Usize
fileStreamTell(FileStream *file)
{
    return fileStreamSeek(file, FileSeekPos_Current, 0);
}

Usize
fileStreamSize(FileStream *file)
{
    if (file->flags & FileStreamFlags_Error) return USIZE_MAX;

    ULARGE_INTEGER size;

    size.LowPart = GetFileSize(file->os_handle, &size.HighPart);

    return (Usize)size.QuadPart;
}
#else

#    include <sys/stat.h>

// NOTE (Matteo): POSIX-like example

FileProperties
fileProperties(Str filename)
{
    FileProperties props = {0};

    Char8 buffer[1024] = {0};
    struct _stat64i32 info = {0};

    memCopy(filename.buf, buffer, filename.len);
    if (!_stat(buffer, &info))
    {
        props.exists = true;
        props.last_write = (SystemTime)info.st_mtime;
        if (info.st_mode & 0x0120000) props.attributes |= FileAttributes_Symlink;
        if (info.st_mode & 0x0040000) props.attributes |= FileAttributes_Directory;
    }

    return props;
}

#endif
