#include "platform.h"

#include "util.h"

#if !defined(_WIN32)
#error "Win32 platform not supported"
#endif // defined(_WIN32)

#pragma warning(push)
#pragma warning(disable : 5105)

#include <windows.h>

#include <stdio.h>

// -----------------------------------------------------------------------------
// Internal function declarations
// -----------------------------------------------------------------------------

// VM functions
static VM_RESERVE_FUNC(win32VmReserve);
static VM_COMMIT_FUNC(win32VmCommit);
static VM_REVERT_FUNC(win32VmDecommit);
static VM_RELEASE_FUNC(win32VmRelease);

// Heap allocation functions
static CF_ALLOCATOR_FUNC(win32Alloc);
static CF_ALLOC_STATS_FUNC(win32AllocStats);

// File system functions
static DirIter *win32DirIterStart(char const *dir, cfAllocator *alloc);
static char const *win32DirIterNext(DirIter *self);
static void win32DirIterClose(DirIter *self);

static char *win32OpenFileDlg(char const *filename_hint, char const *filter, cfAllocator *alloc,
                              u32 *out_size);

// Unicode helpers
static u32 utf8to16Size(char const *str);
static u32 utf16to8Size(WCHAR const *str);
static WCHAR *utf8to16(char const *str, cfAllocator *alloc, u32 *out_size);
static char *utf16to8(WCHAR const *str, cfAllocator *alloc, u32 *out_size);

// -----------------------------------------------------------------------------
// Main API
// -----------------------------------------------------------------------------

cfPlatform
cfPlatformCreate()
{
    SYSTEM_INFO sysinfo;
    GetSystemInfo(&sysinfo);

    cfAllocatorStats *heap_stats =
        HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*heap_stats));

    cfPlatform plat = {.vm = {.reserve = win32VmReserve,
                              .release = win32VmRelease,
                              .commit = win32VmCommit,
                              .revert = win32VmDecommit,
                              .page_size = sysinfo.dwPageSize},
                       .heap =
                           {
                               .state = heap_stats,
                               .reallocate = win32Alloc,
                               .stats = win32AllocStats,
                           },
                       .fs = {
                           .dir_iter_start = win32DirIterStart,
                           .dir_iter_next = win32DirIterNext,
                           .dir_iter_close = win32DirIterClose,
                           .open_file_dlg = win32OpenFileDlg,
                       }};

    return plat;
}

void
cfPlatformShutdown(cfPlatform *platform)
{
    CF_ASSERT_NOT_NULL(platform);

    cfAllocatorStats *heap_stats = platform->heap.state;

    CF_ASSERT_NOT_NULL(heap_stats);
    // TODO (Matteo): Check allocation size tracking
    CF_ASSERT(heap_stats->count == 0, "Potential memory leak");
    CF_ASSERT(heap_stats->size == 0, "Potential memory leak");

    HeapFree(GetProcessHeap(), 0, heap_stats);
}

// -----------------------------------------------------------------------------
// Internal functions
// -----------------------------------------------------------------------------

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
    VirtualFree(memory, size, MEM_DECOMMIT);
}

VM_RELEASE_FUNC(win32VmRelease)
{
    VirtualFree(memory, size, MEM_RELEASE);
}

// -----------------------------------------------------------------------------

CF_ALLOCATOR_FUNC(win32Alloc)
{
    // TODO (Matteo): New memory should be cleared by default?

    HANDLE heap = GetProcessHeap();
    void *old_mem = memory;
    void *new_mem = NULL;

    if (new_size)
    {
        if (old_mem)
        {
            new_mem = HeapReAlloc(heap, 0, old_mem, new_size);
        }
        else
        {
            new_mem = HeapAlloc(heap, 0, new_size);
        }
    }
    else
    {
        HeapFree(heap, 0, old_mem);
    }

    cfAllocatorStats *stats = state;

    if (old_mem)
    {
        CF_ASSERT(old_size > 0, "Freeing valid pointer but given size is 0");
        stats->count--;
        stats->size -= old_size;
    }

    if (new_mem)
    {
        stats->count++;
        stats->size += new_size;
    }

    return new_mem;
}

CF_ALLOC_STATS_FUNC(win32AllocStats)
{
    cfAllocatorStats *stats = state;
    return *stats;
}

// -----------------------------------------------------------------------------

typedef enum Win32DirIterState
{
    Win32DirIterState_Null = 0,
    Win32DirIterState_Start,
    Win32DirIterState_Next,
} Win32DirIterState;

struct DirIter
{
    HANDLE finder;
    char buffer[MAX_PATH];
    u8 state;
    cfAllocator *alloc;
};

DirIter *
win32DirIterStart(char const *dir, cfAllocator *alloc)
{
    DirIter *self = cfAlloc(alloc, sizeof(*self));
    if (!self) return NULL;

    snprintf(self->buffer, MAX_PATH, "%s/*", dir);

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

// -----------------------------------------------------------------------------

u32
utf8to16Size(char const *str)
{
    return MultiByteToWideChar(CP_UTF8, MB_PRECOMPOSED, str, -1, NULL, 0);
}

u32
utf16to8Size(WCHAR const *str)
{
    return WideCharToMultiByte(CP_UTF8, 0, str, -1, NULL, 0, 0, false);
}

WCHAR *
utf8to16(char const *str, cfAllocator *alloc, u32 *out_size)
{
    u32 size = utf8to16Size(str);
    WCHAR *buf = cfAlloc(alloc, size);

    if (buf)
    {
        if (out_size) *out_size = size;
        MultiByteToWideChar(CP_UTF8, MB_PRECOMPOSED, str, -1, buf, size);
    }

    return buf;
}

char *
utf16to8(WCHAR const *str, cfAllocator *alloc, u32 *out_size)
{
    u32 size = utf16to8Size(str);
    char *buf = cfAlloc(alloc, size);

    if (buf)
    {
        if (out_size) *out_size = size;
        WideCharToMultiByte(CP_UTF8, 0, str, -1, buf, size, 0, false);
    }

    return buf;
}

char *
win32OpenFileDlg(char const *filename_hint, char const *filter, cfAllocator *alloc, u32 *out_size)
{

    WCHAR name[MAX_PATH] = {0};

    if (filename_hint)
    {
        u32 name_size = utf8to16Size(filename_hint);
        if (name_size > MAX_PATH) return NULL;
        MultiByteToWideChar(CP_UTF8, MB_PRECOMPOSED, filename_hint, -1, name, name_size);
    }

    u32 filt_size = 0;
    WCHAR *filt = filter ? utf8to16(filter, alloc, &filt_size) : NULL;

    OPENFILENAMEW ofn = {0};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = NULL;
    ofn.lpstrFile = name;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrFilter = filt; // _T("All\0*.*\0Text\0*.TXT\0");
    ofn.nFilterIndex = 1;
    ofn.lpstrFileTitle = NULL;
    ofn.nMaxFileTitle = 0;
    ofn.lpstrInitialDir = NULL;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

    char *result = NULL;

    if (GetOpenFileNameW(&ofn))
    {
        result = utf16to8(ofn.lpstrFile, alloc, out_size);
    }

    cfFree(alloc, filt, filt_size);

    return result;
}

// -----------------------------------------------------------------------------

#pragma warning(pop)
