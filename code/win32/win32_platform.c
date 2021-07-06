#include "win32.h"

#include "api.h"

#include "foundation/core.h"

#include "foundation/fs.h"
#include "foundation/maths.h"
#include "foundation/memory.h"
#include "foundation/paths.h"
#include "foundation/strings.h"

#if !CF_OS_WIN32
#    error "Win32 platform not supported"
#endif

//------------------------------------------------------------------------------
// Cross-platform entry point
//------------------------------------------------------------------------------

I32 platformMain(Platform *platform, char const *argv[], I32 argc);

//------------------------------------------------------------------------------
// Internal implementation
//------------------------------------------------------------------------------

// Virtual memory
static Usize g_vm_page_size = 0;
static VM_RESERVE_FUNC(win32VmReserve);
static VM_COMMIT_FUNC(win32VmCommit);
static VM_REVERT_FUNC(win32VmDecommit);
static VM_RELEASE_FUNC(win32VmRelease);

// Heap allocation
static CF_ALLOCATOR_FUNC(win32Alloc);

// File system
static DirIter *win32DirIterStart(char const *dir, cfAllocator alloc);
static char const *win32DirIterNext(DirIter *self);
static void win32DirIterClose(DirIter *self);

static FileDlgResult win32OpenFileDlg(char const *filename_hint, FileDlgFilter *filters,
                                      Usize num_filters, cfAllocator alloc);

static FileContent win32FileRead(char const *filename, cfAllocator alloc);
static bool win32FileCopy(char const *source, char const *dest, bool overwrite);
static FileTime win32FileWriteTime(char const *filename);

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
static Library *win32libLoad(char const *filename);
static void win32libUnload(Library *lib);
static void *win32libLoadProc(Library *restrict lib, char const *restrict name);

// UTF8<->UTF16 helpers
static U32 win32Utf8To16(char const *str, I32 str_size, WCHAR *out, U32 out_size);
static U32 win32Utf16To8(WCHAR const *str, I32 str_size, char *out, U32 out_size);

static void win32PrintLastError(void);

// Global platform API
// NOTE (Matteo): a global here should be quite safe
static Platform g_platform = {
    .vm =
        &(cfVirtualMemory){
            .reserve = win32VmReserve,
            .release = win32VmRelease,
            .commit = win32VmCommit,
            .revert = win32VmDecommit,
        },
    .heap =
        {
            .func = win32Alloc,
        },
    .fs =
        &(cfFileSystem){
            .dirIterStart = win32DirIterStart,
            .dirIterNext = win32DirIterNext,
            .dirIterClose = win32DirIterClose,
            .open_file_dlg = win32OpenFileDlg,
            .fileRead = win32FileRead,
            .fileCopy = win32FileCopy,
            .fileWrite = win32FileWriteTime,
        },
    .clock = win32Clock,
    .paths = &(Paths){0},
    .libLoad = win32libLoad,
    .libUnload = win32libUnload,
    .libLoadProc = win32libLoadProc,
};

//------------------------------------------------------------------------------
// Main entry point
//------------------------------------------------------------------------------

static void
pathsInit(Paths *g_paths)
{
    // Clear shared buffer
    cfMemClear(g_paths->buffer, CF_ARRAY_SIZE(g_paths->buffer));

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
    strPrintf(g_paths->lib_name.buf, Paths_Size, "%.*s%s", (I32)(g_paths->exe_name.len - ext.len),
              g_paths->exe_name.buf, "_lib.dll");

    g_paths->lib_name.len = strLength(g_paths->lib_name.buf);

    // Build data path from base path
    strPrintf(g_paths->data.buf, Paths_Size, "%.*sdata\\", (I32)g_paths->base.len,
              g_paths->base.buf);
    g_paths->data.len = strLength(g_paths->data.buf);
}

static char **
win32GetCommandLineArgs(cfAllocator alloc, I32 *out_argc, Usize *out_size)
{
    WCHAR *cmd_line = GetCommandLineW();
    WCHAR **argv_utf16 = CommandLineToArgvW(cmd_line, out_argc);

    char **argv = NULL;

    *out_size = (Usize)(*out_argc) * sizeof(*argv) + CF_MB(1);

    argv = cfAlloc(alloc, *out_size);
    char *buf = (char *)(argv + *out_argc);

    for (I32 i = 0; i < *out_argc; ++i)
    {
        argv[i] = buf;
        U32 size = win32Utf16To8(argv_utf16[i], -1, NULL, 0);
        win32Utf16To8(argv_utf16[i], -1, argv[i], size);
        buf += size;
    }

    LocalFree(argv_utf16);

    return argv;
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

    g_vm_page_size = sysinfo.dwPageSize;
    g_platform.vm->page_size = g_vm_page_size;
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
    char **argv = win32GetCommandLineArgs(g_platform.heap, &argc, &cmd_line_size);

    I32 result = platformMain(&g_platform, (char const **)argv, argc);

    cfFree(g_platform.heap, argv, cmd_line_size);

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

//------------------------------------------------------------------------------

#if CF_MEMORY_PROTECTION

static Usize
win32RoundSize(Usize req_size, Usize page_size)
{
    Usize page_count = (req_size + page_size - 1) / page_size;
    return page_count * page_size;
}

CF_ALLOCATOR_FUNC(win32Alloc)
{
    CF_UNUSED(state);

    // TODO (Matteo): Handle alignment? Since this is a replacement of the heap
    // functions I don't think it's necessary to do so.
    CF_UNUSED(align);

    void *new_mem = NULL;

    if (new_size)
    {
        Usize block_size = win32RoundSize(new_size, g_vm_page_size);
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
            cfMemCopy(memory, new_mem, cfMin(old_size, new_size));
        }

        Usize block_size = win32RoundSize(old_size, g_vm_page_size);
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

CF_ALLOCATOR_FUNC(win32Alloc)
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

//------------------------------------------------------------------------------

typedef enum Win32DirIterState
{
    Win32DirIterState_Null = 0,
    Win32DirIterState_Start,
    Win32DirIterState_Next,
} Win32DirIterState;

struct DirIter
{
    cfAllocator alloc;

    HANDLE finder;
    char buffer[MAX_PATH];

    U8 state;
};

DirIter *
win32DirIterStart(char const *dir, cfAllocator alloc)
{
    // TODO (Matteo): Handle UTF8 by converting to WCHAR string

    DirIter *self = cfAlloc(alloc, sizeof(*self));
    if (!self) return NULL;

    strPrintf(self->buffer, MAX_PATH, "%s/*", dir);

    WIN32_FIND_DATAA data = {0};

    self->finder = FindFirstFileA(self->buffer, &data);

    if (self->finder != INVALID_HANDLE_VALUE)
    {
        strncpy_s(self->buffer, MAX_PATH, data.cFileName, MAX_PATH);
        self->state = Win32DirIterState_Start;
        self->alloc = alloc;
        return self;
    }

    cfFree(alloc, self, sizeof(*self));
    return NULL;
}

char const *
win32DirIterNext(DirIter *self)
{
    CF_ASSERT_NOT_NULL(self);

    switch (self->state)
    {
        case Win32DirIterState_Null: return NULL;
        case Win32DirIterState_Start:
        {
            self->state = Win32DirIterState_Next;
            break;
        }
        case Win32DirIterState_Next:
        {
            WIN32_FIND_DATAA data = {0};
            if (!FindNextFileA(self->finder, &data))
            {
                self->state = Win32DirIterState_Null;
                return NULL;
            }
            strncpy_s(self->buffer, MAX_PATH, data.cFileName, MAX_PATH);
        }
    }

    return self->buffer;
}

void
win32DirIterClose(DirIter *self)
{
    CF_ASSERT_NOT_NULL(self);

    if (self->finder != INVALID_HANDLE_VALUE)
    {
        FindClose(self->finder);
    }

    cfFree(self->alloc, self, sizeof(*self));
}

static WCHAR *
win32GrowString(WCHAR *str, U32 len, U32 *cap, U32 req, cfAllocator alloc)
{
    U32 old_cap = *cap;

    if (req > old_cap - len)
    {
        U32 new_cap = cfMax(req, old_cap * 2);
        str = cfRealloc(alloc, str, old_cap, new_cap);
        *cap = new_cap;
    }

    return str;
}

static WCHAR *
win32BuildFilterString(FileDlgFilter *filters, Usize num_filters, cfAllocator alloc, U32 *out_size)
{
    *out_size = 0;
    if (num_filters == 0) return NULL;

    U32 cap = 1024;
    U32 len = 0;
    WCHAR *out_filter = cfAlloc(alloc, cap);

    if (!out_filter) return NULL;

    for (FileDlgFilter *filter = filters, *end = filter + num_filters; filter < end; ++filter)
    {
        U32 name_size = win32Utf8To16(filter->name, -1, NULL, 0);

        out_filter = win32GrowString(out_filter, len, &cap, name_size, alloc);
        if (!out_filter) return NULL;

        win32Utf8To16(filter->name, -1, out_filter + len, name_size);
        len += name_size;

        for (Usize ext_no = 0; ext_no < filter->num_extensions; ++ext_no)
        {
            char const *ext = filter->extensions[ext_no];
            U32 ext_size = win32Utf8To16(ext, -1, NULL, 0);

            out_filter = win32GrowString(out_filter, len, &cap, ext_size + 1, alloc);
            if (!out_filter) return NULL;

            // Prepend '*' to the extension - not documented but actually required
            out_filter[len++] = L'*';
            win32Utf8To16(ext, -1, out_filter + len, ext_size);
            len += ext_size;

            // Replace null terminator with ';' to separate extensions
            out_filter[len - 1] = L';';
        }

        // Append 2 null terminators (required since null terminators are used
        // internally to separate filters)
        out_filter = win32GrowString(out_filter, len, &cap, len + 2, alloc);
        if (!out_filter) return NULL;

        out_filter[len] = 0;
        out_filter[len + 1] = 0;
    }

    *out_size = cap;

    return out_filter;
}

FileDlgResult
win32OpenFileDlg(char const *filename_hint, FileDlgFilter *filters, Usize num_filters,
                 cfAllocator alloc)
{
    FileDlgResult result = {.code = FileDlgResult_Error};

    WCHAR name[MAX_PATH] = {0};

    if (filename_hint)
    {
        U32 name_size = win32Utf8To16(filename_hint, -1, NULL, 0);
        if (name_size > MAX_PATH) return result;
        win32Utf8To16(filename_hint, -1, name, name_size);
    }

    U32 filt_size = 0;
    WCHAR *filt = win32BuildFilterString(filters, num_filters, alloc, &filt_size);

    OPENFILENAMEW ofn = {0};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = NULL;
    ofn.lpstrFile = name;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrFilter = filt; // L"Image files\0*.jpg;*.jpeg;*.bmp;*.png\0";
    ofn.nFilterIndex = 1;
    ofn.lpstrFileTitle = NULL;
    ofn.nMaxFileTitle = 0;
    ofn.lpstrInitialDir = NULL;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

    if (GetOpenFileNameW(&ofn))
    {
        U32 result_size = win32Utf16To8(ofn.lpstrFile, -1, NULL, 0);
        result.filename = cfAlloc(alloc, result_size);

        if (result.filename)
        {
            result.code = FileDlgResult_Ok;
            result.filename_size = result_size;
            win32Utf16To8(ofn.lpstrFile, -1, result.filename, result_size);
        }
    }
    else
    {
        result.code = FileDlgResult_Cancel;
    }

    cfFree(alloc, filt, filt_size);

    return result;
}

FileContent
win32FileRead(char const *filename, cfAllocator alloc)
{
    FileContent result = {0};

    WCHAR path[MAX_PATH] = {0};
    U32 path_size = win32Utf8To16(filename, -1, NULL, 0);

    if (path_size > MAX_PATH)
    {
        CF_ASSERT(false, "filename is too long");
    }
    else
    {
        win32Utf8To16(filename, -1, path, path_size);

        HANDLE file = CreateFileW(path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING,
                                  FILE_ATTRIBUTE_NORMAL, NULL);

        if (file != INVALID_HANDLE_VALUE)
        {
            LARGE_INTEGER file_size;
            GetFileSizeEx(file, &file_size);

            CF_ASSERT(file_size.QuadPart <= ~(DWORD)(0), "File size is too big");

            DWORD read_size = (DWORD)(file_size.QuadPart);
            DWORD read;

            result.data = cfAlloc(alloc, read_size);

            if (result.data && ReadFile(file, result.data, read_size, &read, NULL) &&
                read == read_size)
            {
                result.size = read_size;
            }
            else
            {
                cfFree(alloc, result.data, read_size);
                result.data = NULL;
            }

            CloseHandle(file);
        }
    }

    return result;
}

bool
win32FileCopy(char const *source, char const *dest, bool overwrite)
{
    WCHAR ws[FILENAME_MAX] = {0};
    WCHAR wd[FILENAME_MAX] = {0};
    win32Utf8To16(source, -1, ws, FILENAME_MAX);
    win32Utf8To16(dest, -1, wd, FILENAME_MAX);

    if (CopyFileW(ws, wd, !overwrite)) return true;

    win32PrintLastError();
    return false;
}

static FileTime
win32FileWriteTime(char const *filename)
{
    FileTime write_time = 0;

    WCHAR wide_name[FILENAME_MAX] = {0};
    win32Utf8To16(filename, -1, wide_name, FILENAME_MAX);

    WIN32_FIND_DATAW find_data = {0};
    HANDLE find_handle = FindFirstFileW(wide_name, &find_data);

    if (find_handle != INVALID_HANDLE_VALUE)
    {
        FILETIME temp = find_data.ftLastWriteTime;
        write_time = ((U64)temp.dwHighDateTime << 32) | temp.dwLowDateTime;
        FindClose(find_handle);
    }

    return write_time;
}

//------------------------------------------------------------------------------
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

//------------------------------------------------------------------------------
// Dynamic loading

Library *
win32libLoad(char const *filename)
{
    WCHAR buffer[1024];
    win32Utf8To16(filename, -1, buffer, 1024);
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
win32libLoadProc(Library *lib, char const *name)
{
    return (void *)GetProcAddress((HMODULE)lib, name);
}

//------------------------------------------------------------------------------

U32
win32Utf8To16(char const *str, I32 str_size, WCHAR *out, U32 out_size)
{
    I32 result = MultiByteToWideChar(CP_UTF8, MB_PRECOMPOSED, str, str_size, out, (I32)out_size);
    CF_ASSERT(result >= 0, "");
    return (U32)result;
}

U32
win32Utf16To8(WCHAR const *str, I32 str_size, char *out, U32 out_size)
{
    I32 result = WideCharToMultiByte(CP_UTF8, 0, str, str_size, out, (I32)out_size, 0, false);
    CF_ASSERT(result >= 0, "");
    return (U32)result;
}

//------------------------------------------------------------------------------

void
win32PrintLastError(void)
{
    DWORD error = GetLastError();
    LPSTR msg = NULL;
    FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
                       FORMAT_MESSAGE_IGNORE_INSERTS,
                   NULL, error, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&msg, 0, NULL);
    CF_DEBUG_BREAK();
    fprintf(stderr, "%s\n", msg);
    LocalFree(msg);
}
