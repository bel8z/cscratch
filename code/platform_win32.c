#include "api.h"

#include "foundation/common.h"

#include "foundation/allocator.h"
#include "foundation/fs.h"
#include "foundation/strings.h"
#include "foundation/util.h"
#include "foundation/vm.h"

#if !defined(_WIN32)
#error "Win32 platform not supported"
#endif // defined(_WIN32)

#pragma warning(push)
#pragma warning(disable : 5105)

#include <stdio.h>

#define NOMINMAX 1
#define VC_EXTRALEAN 1
#define WIN32_LEAN_AND_MEAN 1
#include <windows.h>
// Must be included AFTER <windows.h>
#include <commdlg.h>
#include <shellapi.h>

#pragma warning(pop)

//------------------------------------------------------------------------------
// Cross-platform entry point
//------------------------------------------------------------------------------

i32 platform_main(cfPlatform *platform, char const *argv[], i32 argc);

//------------------------------------------------------------------------------
// Internal implementation
//------------------------------------------------------------------------------

// Virtual memory
static usize g_vm_page_size = 0;
static VM_RESERVE_FUNC(win32VmReserve);
static VM_COMMIT_FUNC(win32VmCommit);
static VM_REVERT_FUNC(win32VmDecommit);
static VM_RELEASE_FUNC(win32VmRelease);

// Heap allocation
static CF_ALLOCATOR_FUNC(win32Alloc);

// File system
static DirIter *win32DirIterStart(char const *dir, cfAllocator *alloc);
static char const *win32DirIterNext(DirIter *self);
static void win32DirIterClose(DirIter *self);

static FileDlgResult win32OpenFileDlg(char const *filename_hint, FileDlgFilter *filters,
                                      usize num_filters, cfAllocator *alloc);

static FileContent win32ReadFile(char const *filename, cfAllocator *alloc);

// Timing
static struct
{
    u64 start_ticks;
    u64 ratio_num;
    u64 ratio_den;
    u64 last_ns;
} g_clock;

static u64 win32Clock(void);

// UTF8<->UTF16 helpers
static u32 win32Utf8To16(char const *str, i32 str_size, WCHAR *out, u32 out_size);
static u32 win32Utf16To8(WCHAR const *str, i32 str_size, char *out, u32 out_size);

//------------------------------------------------------------------------------
// Main entry point
//------------------------------------------------------------------------------

char **
win32GetCommandLineArgs(cfAllocator *alloc, i32 *out_argc, usize *out_size)
{
    WCHAR *cmd_line = GetCommandLineW();
    WCHAR **argv_utf16 = CommandLineToArgvW(cmd_line, out_argc);

    char **argv = NULL;

    *out_size = (usize)(*out_argc) * sizeof(*argv) + CF_MB(1);

    argv = cfAlloc(alloc, *out_size);
    char *buf = (char *)(argv + *out_argc);

    for (i32 i = 0; i < *out_argc; ++i)
    {
        argv[i] = buf;
        u32 size = win32Utf16To8(argv_utf16[i], -1, NULL, 0);
        win32Utf16To8(argv_utf16[i], -1, argv[i], size);
        buf += size;
    }

    LocalFree(argv_utf16);

    return argv;
}

#pragma warning(push)
#pragma warning(disable : 4221)

int WINAPI
wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR pCmdLine, int nCmdShow)
{

    CF_UNUSED(hInstance);
    CF_UNUSED(hPrevInstance);
    CF_UNUSED(pCmdLine);
    CF_UNUSED(nCmdShow);

    SYSTEM_INFO sysinfo;
    GetSystemInfo(&sysinfo);

    g_vm_page_size = sysinfo.dwPageSize;

    LARGE_INTEGER now, freq;
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&now);
    CF_ASSERT(freq.QuadPart > 0, "System monotonic clock is not available");
    CF_ASSERT(now.QuadPart > 0, "System monotonic clock is not available");

    g_clock.last_ns = 0;
    g_clock.start_ticks = (u64)now.QuadPart;
    g_clock.ratio_num = (u64)1000000000;
    g_clock.ratio_den = (u64)freq.QuadPart;
    u64 gcd = cfGcd(g_clock.ratio_num, g_clock.ratio_den);
    g_clock.ratio_num /= gcd;
    g_clock.ratio_den /= gcd;

    cfVirtualMemory vm = {
        .reserve = win32VmReserve,
        .release = win32VmRelease,
        .commit = win32VmCommit,
        .revert = win32VmDecommit,
        .page_size = g_vm_page_size,
    };

    cfAllocator heap = {.reallocate = win32Alloc};

    cfFileSystem fs = {
        .dir_iter_start = win32DirIterStart,
        .dir_iter_next = win32DirIterNext,
        .dir_iter_close = win32DirIterClose,
        .open_file_dlg = win32OpenFileDlg,
        .read_file = win32ReadFile,
    };

    cfPlatform platform = {
        .vm = &vm,
        .heap = &heap,
        .fs = &fs,
        .clock = win32Clock,
    };

    heap.state = &platform;

    // TODO (Matteo): Improve command line handling

    usize cmd_line_size;
    i32 argc;
    char **argv = win32GetCommandLineArgs(platform.heap, &argc, &cmd_line_size);

    i32 result = platform_main(&platform, (char const **)argv, argc);

    cfFree(&heap, argv, cmd_line_size);

    CF_ASSERT(platform.heap_blocks == 0, "Potential memory leak");
    CF_ASSERT(platform.heap_size == 0, "Potential memory leak");

    return result;
}

#pragma warning(pop)

//------------------------------------------------------------------------------
// Internal implementation
//------------------------------------------------------------------------------

VM_RESERVE_FUNC(win32VmReserve)
{
    return VirtualAlloc(NULL, size, MEM_RESERVE, PAGE_NOACCESS);
}

VM_COMMIT_FUNC(win32VmCommit)
{
    void *committed = VirtualAlloc(memory, size, MEM_COMMIT, PAGE_READWRITE);
    CF_ASSERT(committed, "Memory not previously reserved");
    return committed != NULL;
}

VM_REVERT_FUNC(win32VmDecommit)
{
    bool result = VirtualFree(memory, size, MEM_DECOMMIT);
    if (!result)
    {
        u32 err = GetLastError();
        CF_UNUSED(err);
        CF_ASSERT(false, "VM decommit failed");
    }
}

VM_RELEASE_FUNC(win32VmRelease)
{
    // NOTE (Matteo): VirtualFree(..., MEM_RELEASE) requires the base pointer
    // returned by VirtualFree(..., MEM_RESERVE) and a size of 0 to succeed.
    CF_UNUSED(size);

    bool result = VirtualFree(memory, 0, MEM_RELEASE);
    if (!result)
    {
        u32 err = GetLastError();
        CF_UNUSED(err);
        CF_ASSERT(false, "VM release failed");
    }
}

//------------------------------------------------------------------------------

#if CF_MEMORY_PROTECTION

static usize
win32RoundSize(usize req_size, usize page_size)
{
    usize page_count = (req_size + page_size - 1) / page_size;
    return page_count * page_size;
}

CF_ALLOCATOR_FUNC(win32Alloc)
{
    cfPlatform *plat = state;

    void *new_mem = NULL;

    if (new_size)
    {
        usize block_size = win32RoundSize(new_size, g_vm_page_size);
        u8 *base = win32VmReserve(block_size);
        if (!base) return NULL;

        win32VmCommit(base, block_size);

        plat->heap_blocks++;
        plat->heap_size += block_size;

        new_mem = base + block_size - new_size;
    }

    if (memory)
    {
        CF_ASSERT(old_size > 0, "Freeing valid pointer but given size is 0");

        if (new_mem)
        {
            cfMemCopy(memory, new_mem, cfMin(old_size, new_size));
        }

        usize block_size = win32RoundSize(old_size, g_vm_page_size);
        u8 *base = (u8 *)memory + old_size - block_size;

        win32VmDecommit(base, block_size);
        win32VmRelease(base, block_size);

        plat->heap_blocks--;
        plat->heap_size -= block_size;

        memory = NULL;
    }

    return new_mem;
}

#else

CF_ALLOCATOR_FUNC(win32Alloc)
{
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

    cfPlatform *plat = state;

    if (old_mem)
    {
        CF_ASSERT(old_size > 0, "Freeing valid pointer but given size is 0");
        plat->heap_blocks--;
        plat->heap_size -= old_size;
    }

    if (new_mem)
    {
        plat->heap_blocks++;
        plat->heap_size += new_size;
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
    cfAllocator *alloc;

    HANDLE finder;
    char buffer[MAX_PATH];

    u8 state;
};

DirIter *
win32DirIterStart(char const *dir, cfAllocator *alloc)
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
win32GrowString(WCHAR *str, u32 len, u32 *cap, u32 req, cfAllocator *alloc)
{
    u32 old_cap = *cap;

    if (req > old_cap - len)
    {
        u32 new_cap = cfMax(req, old_cap * 2);
        str = cfRealloc(alloc, str, old_cap, new_cap);
        *cap = new_cap;
    }

    return str;
}

static WCHAR *
win32BuildFilterString(FileDlgFilter *filters, usize num_filters, cfAllocator *alloc, u32 *out_size)
{
    *out_size = 0;
    if (num_filters == 0) return NULL;

    u32 cap = 1024;
    u32 len = 0;
    WCHAR *out_filter = cfAlloc(alloc, cap);

    if (!out_filter) return NULL;

    for (FileDlgFilter *filter = filters, *end = filter + num_filters; filter < end; ++filter)
    {
        u32 name_size = win32Utf8To16(filter->name, -1, NULL, 0);

        out_filter = win32GrowString(out_filter, len, &cap, name_size, alloc);
        if (!out_filter) return NULL;

        win32Utf8To16(filter->name, -1, out_filter + len, name_size);
        len += name_size;

        for (usize ext_no = 0; ext_no < filter->num_extensions; ++ext_no)
        {
            char const *ext = filter->extensions[ext_no];
            u32 ext_size = win32Utf8To16(ext, -1, NULL, 0);

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
win32OpenFileDlg(char const *filename_hint, FileDlgFilter *filters, usize num_filters,
                 cfAllocator *alloc)
{
    FileDlgResult result = {.code = FileDlgResult_Error};

    WCHAR name[MAX_PATH] = {0};

    if (filename_hint)
    {
        u32 name_size = win32Utf8To16(filename_hint, -1, NULL, 0);
        if (name_size > MAX_PATH) return result;
        win32Utf8To16(filename_hint, -1, name, name_size);
    }

    u32 filt_size = 0;
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
        u32 result_size = win32Utf16To8(ofn.lpstrFile, -1, NULL, 0);
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
win32ReadFile(char const *filename, cfAllocator *alloc)
{
    FileContent result = {0};

    WCHAR path[MAX_PATH] = {0};
    u32 path_size = win32Utf8To16(filename, -1, NULL, 0);

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

//------------------------------------------------------------------------------
u64
win32Clock(void)
{
    LARGE_INTEGER now;
    QueryPerformanceCounter(&now);

    CF_ASSERT((u64)now.QuadPart > g_clock.start_ticks, "Win32 clock is not monotonic!");

    u64 time_delta = (u64)now.QuadPart - g_clock.start_ticks;
    u64 ns = (time_delta * g_clock.ratio_num) / g_clock.ratio_den;

    CF_ASSERT(g_clock.last_ns < ns, "Win32 clock wrapped");
    g_clock.last_ns = ns;

    return ns;
}

//------------------------------------------------------------------------------

u32
win32Utf8To16(char const *str, i32 str_size, WCHAR *out, u32 out_size)
{
    i32 result = MultiByteToWideChar(CP_UTF8, MB_PRECOMPOSED, str, str_size, out, (i32)out_size);
    CF_ASSERT(result >= 0, "");
    return (u32)result;
}

u32
win32Utf16To8(WCHAR const *str, i32 str_size, char *out, u32 out_size)
{
    i32 result = WideCharToMultiByte(CP_UTF8, 0, str, str_size, out, (i32)out_size, 0, false);
    CF_ASSERT(result >= 0, "");
    return (u32)result;
}

//------------------------------------------------------------------------------
