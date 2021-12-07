#include "fs.h"
#include "memory.h"
#include "strings.h"

//-----------------------//
//   Common operations   //
//-----------------------//

FileContent
fileReadContent(Str filename, MemAllocator alloc)
{
    FileContent result = {0};

    File file = fileOpen(filename, FileOpenMode_Read);

    if (!file.error)
    {
        Usize file_size = file.size(&file);
        Usize read_size = file_size;

        result.data = memAlloc(alloc, read_size);

        if (result.data && file.read(&file, result.data, read_size) == read_size)
        {
            result.size = read_size;
        }
        else
        {
            memFree(alloc, result.data, read_size);
            result.data = NULL;
        }

        fileClose(&file);
    }

    return result;
}

bool
fileWriteStr(File *file, Str str)
{
    return file->write(file, (U8 const *)str.buf, str.len);
}

#if 0
static Usize
findLineBreak(U8 const *buffer, Usize size)
{
    bool cr_found = false;

    for (Usize pos = 0; pos < size; ++pos)
    {
        switch (buffer[pos])
        {
            case '\r': cr_found = true; break;
            case '\n': return cr_found ? pos - 1 : pos;
            default: cr_found = false; break;
        }
    }

    return USIZE_MAX;
}
#endif

//--------------------------------//
//   OS-specific implementation   //
//--------------------------------//

#ifdef CF_OS_WIN32

#    include "win32.inl"

// TODO (Matteo): Provide async file IO? Win32 offers IO Completion Ports which seem very good for
// the purpose

typedef struct Win32DirIterator
{
    HANDLE finder;
    WIN32_FIND_DATAW data;
    Char8 buffer[sizeof(FsIterator) - sizeof(HANDLE) - sizeof(WIN32_FIND_DATAW)];
} Win32DirIterator;

// NOTE (Matteo): Ensure that there is room for a reasonably sized buffer
static_assert(sizeof(((Win32DirIterator *)0)->buffer) >= 512,
              "FsIterator buffer size is too small");

static inline U64
win32MergeWords(DWORD high, DWORD low)
{
    ULARGE_INTEGER value = {.HighPart = high, .LowPart = low};
    return value.QuadPart;
}

static inline SystemTime
win32FileSystemTime(FILETIME time)
{
    return win32MergeWords(time.dwHighDateTime, time.dwLowDateTime);
}

static void
win32ReadAttributes(DWORD attributes, FileProperties *out)
{
    if (attributes & FILE_ATTRIBUTE_DIRECTORY)
    {
        out->attributes |= FileAttributes_Directory;
    }

    if (attributes & FILE_ATTRIBUTE_REPARSE_POINT)
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

static FS_ITERATOR_NEXT(fsIteratorNext)
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

    if (props)
    {
        props->exists = true;
        props->last_write = win32FileSystemTime(iter->data.ftLastWriteTime);
        props.size = (Usize)win32MergeWords(iter->data.nFileSizeHigh, iter->data.nFileSizeLow);
        win32ReadAttributes(iter->data.dwFileAttributes, props);
    }

    return true;
}

bool
fsIteratorStart(FsIterator *self, Str dir_path)
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

    self->next = fsIteratorNext;

    return true;
}

void
fsIteratorEnd(FsIterator *self)
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

    win32HandleLastError();
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
        props.exists = true;
        props.last_write = win32FileSystemTime(data.ftLastWriteTime);
        props.size = (Usize)win32MergeWords(data.nFileSizeHigh, data.nFileSizeLow);
        win32ReadAttributes(data.dwFileAttributes, &props);
        FindClose(find_handle);
    }

    return props;
}

static FILE_READ(win32FileRead)
{
    if (file->error) return USIZE_MAX;
    if (file->eof) return 0;

    DWORD read_bytes;

    if (!ReadFile(file->os_handle, buffer, (DWORD)buffer_size, &read_bytes, NULL))
    {
        file->error = true;
        win32HandleLastError();
        return USIZE_MAX;
    }
    else if (read_bytes < buffer_size)
    {
        file->eof = true;
    }

    return read_bytes;
}

static FILE_READ_AT(win32FileReadAt)
{
    if (file->error) return USIZE_MAX;

    OVERLAPPED overlapped = win32FileOffset(offset);
    DWORD read_bytes;

    if (!ReadFile(file->os_handle, buffer, (DWORD)buffer_size, &read_bytes, &overlapped))
    {
        file->error = true;
        win32HandleLastError();
        return USIZE_MAX;
    }

    return read_bytes;
}

static FILE_WRITE(win32FileWrite)
{
    if (file->error) return false;

    DWORD written_bytes;

    if (!WriteFile(file->os_handle, data, (DWORD)data_size, &written_bytes, NULL))
    {
        file->error = true;
        win32HandleLastError();
        return false;
    }

    CF_ASSERT(written_bytes == (DWORD)data_size, "Incorrect number of bytes written");

    return true;
}

static FILE_WRITE_AT(win32FileWriteAt)
{
    if (file->error) return false;

    OVERLAPPED overlapped = win32FileOffset(offset);
    DWORD written_bytes;

    if (!WriteFile(file->os_handle, data, (DWORD)data_size, &written_bytes, &overlapped))
    {
        file->error = true;
        win32HandleLastError();
        return false;
    }

    CF_ASSERT(written_bytes == (DWORD)data_size, "Incorrect number of bytes written");

    return true;
}

static FILE_SEEK(win32FileSeek)
{
    LARGE_INTEGER temp = {.QuadPart = (LONGLONG)offset};
    LARGE_INTEGER dest = {0};

    if (!SetFilePointerEx(file->os_handle, temp, &dest, pos))
    {
        file->error = true;
        win32HandleLastError();
    };

    return (Usize)dest.QuadPart;
}

static FILE_TELL(win32FileTell)
{
    return win32FileSeek(file, FileSeekPos_Current, 0);
}

static FILE_SIZE(win32FileSize)
{
    if (file->error) return USIZE_MAX;

    ULARGE_INTEGER size;

    size.LowPart = GetFileSize(file->os_handle, &size.HighPart);

    return (Usize)size.QuadPart;
}

static FILE_PROPERTIES(win32FileProperties)
{
    FileProperties props = {0};
    BY_HANDLE_FILE_INFORMATION info;

    if (GetFileInformationByHandle(file->os_handle, &info))
    {
        props.exists = true;
        props.last_write = win32FileSystemTime(info.ftLastWriteTime);
        props.size = (Usize)win32MergeWords(info.nFileSizeHigh, info.nFileSizeLow);
        win32ReadAttributes(info.dwFileAttributes, &props);
    }

    return props;
}

File
fileOpen(Str filename, FileOpenMode mode)
{
    File result = {
        .read = win32FileRead,
        .readAt = win32FileReadAt,
        .seek = win32FileSeek,
        .size = win32FileSize,
        .tell = win32FileTell,
        .write = win32FileWrite,
        .writeAt = win32FileWriteAt,
        .properties = win32FileProperties,
    };

    if (mode == 0)
    {
        result.error = true;
    }
    else
    {
        DWORD access = 0;
        DWORD creation = OPEN_EXISTING;

        if (mode & FileOpenMode_Read) access |= GENERIC_READ;

        if (mode & FileOpenMode_Write)
        {
            access |= GENERIC_WRITE;
            // Overwrite creation mode
            creation = CREATE_ALWAYS;
        }

        Char16 buffer[1024] = {0};
        win32Utf8To16(filename, buffer, CF_ARRAY_SIZE(buffer));

        result.os_handle = CreateFileW(buffer, access, FILE_SHARE_READ, NULL, creation,
                                       FILE_ATTRIBUTE_NORMAL, NULL);

        if (result.os_handle == INVALID_HANDLE_VALUE)
        {
            result.error = true;
            win32HandleLastError();
        }
        else if (mode == FileOpenMode_Append)
        {
            if (!SetFilePointer(result.os_handle, 0, NULL, FILE_END))
            {
                CloseHandle(result.os_handle);
                result.os_handle = INVALID_HANDLE_VALUE;
                result.error = true;
                win32HandleLastError();
            }
        }
    }

    return result;
}

void
fileClose(File *file)
{
    CloseHandle(file->os_handle);
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
