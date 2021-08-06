#include "api.h"

#include "foundation/array.h"
#include "foundation/core.h"
#include "foundation/fs.h"
#include "foundation/maths.h"
#include "foundation/memory.h"
#include "foundation/paths.h"
#include "foundation/strings.h"
#include "foundation/win32.h"

//------------------------------------------------------------------------------
// Cross-platform entry point
//------------------------------------------------------------------------------

I32 platformMain(Platform *platform, Cstr argv[], I32 argc);

//------------------------------------------------------------------------------
// API implementation
//------------------------------------------------------------------------------

// Virtual memory
static VM_RESERVE_FUNC(win32VmReserve);
static VM_COMMIT_FUNC(win32VmCommit);
static VM_REVERT_FUNC(win32VmDecommit);
static VM_RELEASE_FUNC(win32VmRelease);

static VM_MIRROR_ALLOCATE(win32MirrorAllocate);
static VM_MIRROR_FREE(win32MirrorFree);

// Heap allocation
static MEM_ALLOCATOR_FUNC(win32Alloc);

// File system
typedef struct Win32DirIterator
{
    HANDLE finder;
    Char8 buffer[sizeof(DirIterator) - sizeof(HANDLE)];
} Win32DirIterator;

bool win32DirIterStart(DirIterator *self, Str dir_path);
bool win32DirIterNext(DirIterator *self, Str *filename);
void win32DirIterEnd(DirIterator *self);

static FileContent win32FileRead(Str filename, MemAllocator alloc);
static bool win32FileCopy(Str source, Str dest, bool overwrite);
static FileProperties win32FileProperties(Str filename);

// Timing
static struct
{
    I64 start_ticks;
    I64 ratio_num;
    I64 ratio_den;
    I64 last_ns;
} g_clock;

static Time win32Clock(void);

/// Dynamic loading
static Library *win32libLoad(Str filename);
static void win32libUnload(Library *lib);
static void *win32libLoadProc(Library *restrict lib, Cstr restrict name);

// Global platform API
// NOTE (Matteo): a global here should be quite safe
static Platform g_platform = {
    .vm =
        &(CfVirtualMemory){
            .reserve = win32VmReserve,
            .release = win32VmRelease,
            .commit = win32VmCommit,
            .revert = win32VmDecommit,
            .mirrorAllocate = win32MirrorAllocate,
            .mirrorFree = win32MirrorFree,
        },
    .heap =
        {
            .func = win32Alloc,
        },
    .fs =
        &(CfFileSystem){
            .dirIterStart = win32DirIterStart,
            .dirIterNext = win32DirIterNext,
            .dirIterEnd = win32DirIterEnd,
            .fileRead = win32FileRead,
            .fileCopy = win32FileCopy,
            .fileProperties = win32FileProperties,
        },
    .clock = win32Clock,
    .paths = &(Paths){0},
    .libLoad = win32libLoad,
    .libUnload = win32libUnload,
    .libLoadProc = win32libLoadProc,
};

//------------------------------------------------------------------------------
// Utilities
//------------------------------------------------------------------------------

static Char8 **
win32GetCommandLineArgs(MemAllocator alloc, I32 *out_argc, Usize *out_size)
{
    Char16 *cmd_line = GetCommandLineW();
    Char16 **argv_utf16 = CommandLineToArgvW(cmd_line, out_argc);

    Char8 **argv = NULL;

    *out_size = (Usize)(*out_argc) * sizeof(*argv) + CF_MB(1);

    argv = memAlloc(alloc, *out_size);
    Char8 *buf = (Char8 *)(argv + *out_argc);

    for (I32 i = 0; i < *out_argc; ++i)
    {
        argv[i] = buf;
        Usize size = win32Utf16To8C(argv_utf16[i], NULL, 0);
        win32Utf16To8C(argv_utf16[i], argv[i], size);
        buf += size;
    }

    LocalFree(argv_utf16);

    return argv;
}

//------------------------------------------------------------------------------
// Main entry point
//------------------------------------------------------------------------------

static void
pathsInit(Paths *g_paths)
{
    // Clear shared buffer
    memClear(g_paths->buffer, CF_ARRAY_SIZE(g_paths->buffer));

    // Point string views to assigned positions
    g_paths->base.buf = g_paths->buffer;
    g_paths->lib_name.buf = g_paths->buffer + 1 * Paths_Size;
    g_paths->data.buf = g_paths->buffer + 2 * Paths_Size;

    // Retrieve executable full path
    g_paths->base.len = GetModuleFileNameA(0, g_paths->buffer, Paths_Size);
    CF_ASSERT(g_paths->base.len < Paths_Size, "Executable path is too long");

    // Split executable full path
    Str ext;
    g_paths->exe_name = pathSplitNameExt(g_paths->base, &ext);
    CF_ASSERT(strValid(g_paths->exe_name), "Invalid executable file name");
    CF_ASSERT(strValid(ext), "Invalid executable file name");
    g_paths->base.len -= g_paths->exe_name.len;

    // Build library filename
    strPrintf((Char8 *)g_paths->lib_name.buf, Paths_Size, "%.*s%s",
              (I32)(g_paths->exe_name.len - ext.len), g_paths->exe_name.buf, "_lib.dll");

    g_paths->lib_name.len = strLength(g_paths->lib_name.buf);

    // Build data path from base path
    strPrintf((Char8 *)g_paths->data.buf, Paths_Size, "%.*sdata\\", (I32)g_paths->base.len,
              g_paths->base.buf);
    g_paths->data.len = strLength(g_paths->data.buf);
}

int WINAPI
wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR pCmdLine, int nCmdShow)
{
    CF_UNUSED(hInstance);
    CF_UNUSED(hPrevInstance);
    CF_UNUSED(pCmdLine);
    CF_UNUSED(nCmdShow);

    // ** Init memory management **

    SYSTEM_INFO sysinfo;
    GetSystemInfo(&sysinfo);

    g_platform.vm->page_size = sysinfo.dwPageSize;
    g_platform.vm->address_granularity = sysinfo.dwAllocationGranularity;
    g_platform.heap.state = &g_platform;

    // ** Init timing **

    LARGE_INTEGER now, freq;
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&now);
    CF_ASSERT(freq.QuadPart > 0, "System monotonic clock is not available");
    CF_ASSERT(now.QuadPart > 0, "System monotonic clock is not available");

    g_clock.last_ns = 0;
    g_clock.start_ticks = now.QuadPart;
    g_clock.ratio_num = 1000000000;
    g_clock.ratio_den = freq.QuadPart;
    U64 gcd = cfGcd((U64)g_clock.ratio_num, (U64)g_clock.ratio_den);
    g_clock.ratio_num /= gcd;
    g_clock.ratio_den /= gcd;

    // ** Init paths **

    pathsInit(g_platform.paths);

    // ** Platform-agnostic entry point **

    // TODO (Matteo): Improve command line handling

    Usize cmd_line_size;
    I32 argc;
    Char8 **argv = win32GetCommandLineArgs(g_platform.heap, &argc, &cmd_line_size);

    I32 result = platformMain(&g_platform, (Cstr *)argv, argc);

    cfMemFree(g_platform.heap, argv, cmd_line_size);

    CF_ASSERT(g_platform.heap_blocks == 0, "Potential memory leak");
    CF_ASSERT(g_platform.heap_size == 0, "Potential memory leak");

    return result;
}

//------------------------------------------------------------------------------
// Internal implementation
//------------------------------------------------------------------------------

VM_RESERVE_FUNC(win32VmReserve)
{
    void *mem = VirtualAlloc(NULL, size, MEM_RESERVE, PAGE_NOACCESS);

    if (mem)
    {
        g_platform.reserved_size += size;
    }

    return mem;
}

VM_COMMIT_FUNC(win32VmCommit)
{
    void *committed = VirtualAlloc(memory, size, MEM_COMMIT, PAGE_READWRITE);

    if (committed)
    {
        g_platform.committed_size += size;
        return true;
    }

    CF_ASSERT(committed, "Memory not previously reserved");
    return false;
}

VM_REVERT_FUNC(win32VmDecommit)
{
    bool result = VirtualFree(memory, size, MEM_DECOMMIT);
    if (result)
    {
        g_platform.committed_size -= size;
    }
    else
    {
        win32PrintLastError();
        CF_ASSERT(false, "VM decommit failed");
    }
}

VM_RELEASE_FUNC(win32VmRelease)
{
    // NOTE (Matteo): VirtualFree(..., MEM_RELEASE) requires the base pointer
    // returned by VirtualFree(..., MEM_RESERVE) and a size of 0 to succeed.
    CF_UNUSED(size);

    bool result = VirtualFree(memory, 0, MEM_RELEASE);
    if (result)
    {
        g_platform.reserved_size -= size;
    }
    else
    {
        win32PrintLastError();
        CF_ASSERT(false, "VM release failed");
    }
}

VM_MIRROR_ALLOCATE(win32MirrorAllocate)
{
    // NOTE (Matteo): Size is rounded to virtual memory granularity because the mapping addresses
    // must be aligned as such.
    Usize granularity = g_platform.vm->address_granularity;
    Usize buffer_size = (size + granularity - 1) & ~(granularity - 1);

    VmMirrorBuffer buffer = {0};

    HANDLE mapping =
        CreateFileMappingW(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, (DWORD)(buffer_size >> 32),
                           (DWORD)(buffer_size & 0xffffffff), NULL);

    if (mapping)
    {
        buffer.size = buffer_size;
        buffer.os_handle = mapping;

        while (!buffer.data)
        {
            U8 *address = VirtualAlloc(NULL, buffer_size * 2, MEM_RESERVE, PAGE_READWRITE);
            if (address)
            {
                VirtualFree(address, 0, MEM_RELEASE);

                U8 *view1 =
                    MapViewOfFileEx(mapping, FILE_MAP_ALL_ACCESS, 0, 0, buffer_size, address);
                U8 *view2 = MapViewOfFileEx(mapping, FILE_MAP_ALL_ACCESS, 0, 0, buffer_size,
                                            view1 + buffer_size);

                if (view1 && view2)
                {
                    buffer.data = view1;
                }
                else
                {
                    win32PrintLastError();
                }
            }
        }

        memClear(buffer.data, buffer.size);
    }

    return buffer;
}

VM_MIRROR_FREE(win32MirrorFree)
{
    if (buffer->data)
    {
        UnmapViewOfFile((U8 *)buffer->data + buffer->size);
        UnmapViewOfFile((U8 *)buffer->data);
    }

    if (buffer->os_handle) CloseHandle(buffer->os_handle);

    buffer->size = 0;
    buffer->data = 0;
    buffer->os_handle = 0;
}

#if CF_MEMORY_PROTECTION

static Usize
win32RoundSize(Usize req_size, Usize page_size)
{
    Usize page_count = (req_size + page_size - 1) / page_size;
    return page_count * page_size;
}

MEM_ALLOCATOR_FUNC(win32Alloc)
{
    CF_UNUSED(state);

    // TODO (Matteo): Handle alignment? Since this is a replacement of the heap
    // functions I don't think it's necessary to do so.
    CF_UNUSED(align);

    void *new_mem = NULL;

    if (new_size)
    {
        Usize block_size = win32RoundSize(new_size, g_platform.vm->page_size);
        U8 *base = win32VmReserve(block_size);
        if (!base) return NULL;

        win32VmCommit(base, block_size);

        g_platform.heap_blocks++;
        g_platform.heap_size += block_size;

        new_mem = base + block_size - new_size;
    }

    if (memory)
    {
        CF_ASSERT(old_size > 0, "Freeing valid pointer but given size is 0");

        if (new_mem)
        {
            memCopy(memory, new_mem, cfMin(old_size, new_size));
        }

        Usize block_size = win32RoundSize(old_size, g_platform.vm->page_size);
        U8 *base = (U8 *)memory + old_size - block_size;

        win32VmDecommit(base, block_size);
        win32VmRelease(base, block_size);

        g_platform.heap_blocks--;
        g_platform.heap_size -= block_size;

        memory = NULL;
    }

    return new_mem;
}

#else

MEM_ALLOCATOR_FUNC(win32Alloc)
{
    CF_UNUSED(state);
    CF_UNUSED(align);

    HANDLE heap = GetProcessHeap();
    void *old_mem = memory;
    void *new_mem = NULL;

    if (new_size)
    {
        if (old_mem)
        {
            new_mem = HeapReAlloc(heap, HEAP_ZERO_MEMORY, old_mem, new_size);
        }
        else
        {
            new_mem = HeapAlloc(heap, HEAP_ZERO_MEMORY, new_size);
        }
    }
    else
    {
        HeapFree(heap, 0, old_mem);
    }

    if (old_mem)
    {
        CF_ASSERT(old_size > 0, "Freeing valid pointer but given size is 0");
        g_platform.heap_blocks--;
        g_platform.heap_size -= old_size;
    }

    if (new_mem)
    {
        g_platform.heap_blocks++;
        g_platform.heap_size += new_size;
    }

    return new_mem;
}

#endif

bool
win32DirIterStart(DirIterator *self, Str dir_path)
{
    CF_ASSERT_NOT_NULL(self);

    Win32DirIterator *iter = (Win32DirIterator *)self->opaque;
    WIN32_FIND_DATAW data = {0};
    Char16 buffer[1024];

    // Encode path to UTF16
    Usize size = win32Utf8To16(dir_path, buffer, CF_ARRAY_SIZE(buffer));

    if (size == USIZE_MAX || (Usize)size >= CF_ARRAY_SIZE(buffer) - 2)
    {
        CF_ASSERT(false, "Encoding error or overflow");
        return false;
    }

    // Append a wildcard (D:)
    buffer[size - 1] = L'*';
    buffer[size] = 0;

    // Start iteration
    iter->finder = FindFirstFileW(buffer, &data);
    if (iter->finder == INVALID_HANDLE_VALUE) return false;

    // Skip to ".." so that the "advance" method can call FindNextFileW directly
    if (!wcscmp(data.cFileName, L".") && !FindNextFileW(iter->finder, &data))
    {
        FindClose(iter->finder);
        iter->finder = INVALID_HANDLE_VALUE;
        return false;
    }

    return true;
}

bool
win32DirIterNext(DirIterator *self, Str *filename)
{
    CF_ASSERT_NOT_NULL(self);

    Win32DirIterator *iter = (Win32DirIterator *)self->opaque;
    WIN32_FIND_DATAW data = {0};

    if (!FindNextFileW(iter->finder, &data)) return false;

    Usize size = win32Utf16To8C(data.cFileName, iter->buffer, CF_ARRAY_SIZE(iter->buffer));

    // NOTE (Matteo): Truncation is considered an error
    // TODO (Matteo): Maybe require a bigger buffer?
    if (size == USIZE_MAX || size == CF_ARRAY_SIZE(iter->buffer)) return false;

    CF_ASSERT(size > 0, "Which filename can have a size of 0???");

    filename->buf = iter->buffer;
    filename->len = (Usize)(size - 1);

    return true;
}

void
win32DirIterEnd(DirIterator *self)
{
    CF_ASSERT_NOT_NULL(self);

    Win32DirIterator *iter = (Win32DirIterator *)self->opaque;
    FindClose(iter->finder);
}

FileContent
win32FileRead(Str filename, MemAllocator alloc)
{
    FileContent result = {0};

    Char16 path[MAX_PATH] = {0};
    Usize path_len = win32Utf8To16(filename, NULL, 0);

    if (path_len >= CF_ARRAY_SIZE(path))
    {
        CF_ASSERT(false, "filename is too long");
    }
    else
    {
        win32Utf8To16(filename, path, CF_ARRAY_SIZE(path));

        HANDLE file = CreateFileW(path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING,
                                  FILE_ATTRIBUTE_NORMAL, NULL);

        if (file != INVALID_HANDLE_VALUE)
        {
            LARGE_INTEGER file_size;
            GetFileSizeEx(file, &file_size);

            CF_ASSERT(file_size.QuadPart <= ~(DWORD)(0), "File size is too big");

            DWORD read_size = (DWORD)(file_size.QuadPart);
            DWORD read;

            result.data = memAlloc(alloc, read_size);

            if (result.data && ReadFile(file, result.data, read_size, &read, NULL) &&
                read == read_size)
            {
                result.size = read_size;
            }
            else
            {
                cfMemFree(alloc, result.data, read_size);
                result.data = NULL;
            }

            CloseHandle(file);
        }
    }

    return result;
}

bool
win32FileCopy(Str source, Str dest, bool overwrite)
{
    Char16 ws[MAX_PATH] = {0};
    Char16 wd[MAX_PATH] = {0};
    win32Utf8To16(source, ws, CF_ARRAY_SIZE(ws));
    win32Utf8To16(dest, wd, CF_ARRAY_SIZE(wd));

    if (CopyFileW(ws, wd, !overwrite)) return true;

    win32PrintLastError();
    return false;
}

#if 1

// NOTE (Matteo): Win32 native

static FileProperties
win32FileProperties(Str filename)
{
    FileProperties props = {0};

    Char16 wide_name[MAX_PATH] = {0};
    win32Utf8To16(filename, wide_name, CF_ARRAY_SIZE(wide_name));

    WIN32_FIND_DATAW data = {0};
    HANDLE find_handle = FindFirstFileW(wide_name, &data);

    if (find_handle != INVALID_HANDLE_VALUE)
    {
        props.exists = true;

        FILETIME write_time = data.ftLastWriteTime;
        props.last_write = ((U64)write_time.dwHighDateTime << 32) | write_time.dwLowDateTime;

        if (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        {
            props.attributes |= FileAttributes_Directory;
        }

        if (data.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT)
        {
            props.attributes |= FileAttributes_Symlink;
        }

        FindClose(find_handle);
    }

    return props;
}

#else
#    include <sys/stat.h>

// NOTE (Matteo): POSIX-like example

static FileProperties
win32FileProperties(Str filename)
{
    FileProperties props = {0};

    Char8 buffer[1024] = {0};
    struct _stat64i32 info = {0};

    cfMemCopy(filename.buf, buffer, filename.len);
    if (!_stat(buffer, &info))
    {
        props.exists = true;
        props.last_write = (FileTime)info.st_mtime;
        if (info.st_mode & 0x0120000) props.attributes |= FileAttributes_Symlink;
        if (info.st_mode & 0x0040000) props.attributes |= FileAttributes_Directory;
    }

    return props;
}

#endif

Time
win32Clock(void)
{
    LARGE_INTEGER now;
    QueryPerformanceCounter(&now);

    CF_ASSERT(now.QuadPart > g_clock.start_ticks, "Win32 clock is not monotonic!");

    I64 time_delta = now.QuadPart - g_clock.start_ticks;
    Time time = {.nanoseconds = (time_delta * g_clock.ratio_num) / g_clock.ratio_den};

    CF_ASSERT(g_clock.last_ns < time.nanoseconds, "Win32 clock wrapped");
    g_clock.last_ns = time.nanoseconds;

    return time;
}

Library *
win32libLoad(Str filename)
{
    Char16 buffer[1024] = {0};
    win32Utf8To16(filename, buffer, CF_ARRAY_SIZE(buffer));
    Library *lib = (void *)LoadLibraryW(buffer);

    if (!lib)
    {
        win32PrintLastError();
        CF_ASSERT(FALSE, "LoadLibraryW FAILED");
    }

    return lib;
}

void
win32libUnload(Library *lib)
{
    if (!FreeLibrary((HMODULE)lib))
    {
        win32PrintLastError();
        CF_ASSERT(FALSE, "FreeLibrary FAILED");
    }
}

void *
win32libLoadProc(Library *lib, Cstr name)
{
    return (void *)GetProcAddress((HMODULE)lib, name);
}

//-----------------------------------------------------------------------------
