#include "fs.h"
#include "memory.h"
#include "strings.h"

//-----------------------//
//   Common operations   //
//-----------------------//

FileContent
fileReadContent(FileApi *api, Str filename, MemAllocator alloc)
{
    FileContent result = {0};

    File *file = api->open(filename, FileOpenMode_Read);

    if (file != api->invalid)
    {
        Usize file_size = api->size(file);
        Usize read_size = file_size;

        result.data = memAlloc(alloc, read_size);

        if (result.data && api->read(file, result.data, read_size) == read_size)
        {
            result.size = read_size;
        }
        else
        {
            memFree(alloc, result.data, read_size);
            result.data = NULL;
        }

        api->close(file);
    }

    return result;
}

bool
fileWriteStr(FileApi *api, File *file, Str str)
{
    return api->write(file, (U8 const *)str.buf, str.len);
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
        props->size = (Usize)win32MergeWords(iter->data.nFileSizeHigh, iter->data.nFileSizeLow);
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
