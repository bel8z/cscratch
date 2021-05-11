#include "platform.h"

#include "util.h"

#if !defined(_WIN32)
#error "Win32 platform not supported"
#endif // defined(_WIN32)

#pragma warning(push)
#pragma warning(disable : 5105)

#include <windows.h>

#include <stdio.h>

//------------------------------------------------------------------------------
// Internal function declarations
//------------------------------------------------------------------------------

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

static char *win32OpenFileDlg(char const *filename_hint, FileDlgFilter *filters, usize num_filters,
                              cfAllocator *alloc, u32 *out_size);

// Unicode helpers
static u32 win32Utf8To16(char const *str, i32 str_size, WCHAR *out, u32 out_size);
static u32 win32Utf16To8(WCHAR const *str, i32 str_size, char *out, u32 out_size);

//------------------------------------------------------------------------------
// Main API
//------------------------------------------------------------------------------

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

//------------------------------------------------------------------------------
// Internal functions
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

usize
win32RoundSize(usize req_size, usize page_size)
{
    usize page_count = (req_size + page_size - 1) / page_size;
    return page_count * page_size;
}

CF_ALLOCATOR_FUNC(win32Alloc)
{
    cfAllocatorStats *stats = state;

    SYSTEM_INFO sysinfo;
    GetSystemInfo(&sysinfo);

    usize page_size = sysinfo.dwPageSize;

    void *new_mem = NULL;

    if (new_size)
    {
        usize block_size = win32RoundSize(new_size, page_size);
        u8 *base = win32VmReserve(block_size);
        if (!base) return NULL;

        win32VmCommit(base, block_size);

        stats->count++;
        stats->size += block_size;

        new_mem = base + block_size - new_size;
    }

    if (memory)
    {
        CF_ASSERT(old_size > 0, "Freeing valid pointer but given size is 0");

        if (new_mem)
        {
            cfMemCopy(memory, new_mem, cfMin(old_size, new_size));
        }

        usize block_size = win32RoundSize(old_size, page_size);
        u8 *base = (u8 *)memory + old_size - block_size;

        win32VmDecommit(base, block_size);
        win32VmRelease(base, block_size);

        stats->count--;
        stats->size -= block_size;

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

#endif

CF_ALLOC_STATS_FUNC(win32AllocStats)
{
    cfAllocatorStats *stats = state;
    return *stats;
}

//------------------------------------------------------------------------------

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

static WCHAR *
win32GrowString(WCHAR *str, usize len, usize *cap, usize req, cfAllocator *alloc)
{
    usize old_cap = *cap;

    if (req > old_cap - len)
    {
        usize new_cap = cfMax(req, old_cap * 2);
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

    usize cap = 1024;
    usize len = 0;
    WCHAR *out_filter = cfAlloc(alloc, cap);

    if (!out_filter) return NULL;

    for (usize filter_no = 0; filter_no < num_filters; ++filter_no)
    {
        usize name_size = win32Utf8To16(filters[filter_no].name, -1, NULL, 0);

        out_filter = win32GrowString(out_filter, len, &cap, name_size, alloc);
        if (!out_filter) return NULL;

        win32Utf8To16(filters[filter_no].name, -1, out_filter + len, name_size);
        len += name_size;

        for (usize ext_no = 0; ext_no < filters[filter_no].num_extensions; ++ext_no)
        {
            char const *ext = filters[filter_no].extensions[ext_no];
            usize ext_size = win32Utf8To16(ext, -1, NULL, 0);

            out_filter = win32GrowString(out_filter, len, &cap, ext_size + 1, alloc);
            if (!out_filter) return NULL;

            out_filter[len++] = L'*';
            win32Utf8To16(ext, -1, out_filter + len, ext_size);
            len += ext_size;

            out_filter[len - 1] = L';';
        }

        out_filter = win32GrowString(out_filter, len, &cap, len + 2, alloc);
        if (!out_filter) return NULL;

        out_filter[len] = 0;
        out_filter[len + 1] = 0;
    }

    *out_size = cap;

    return out_filter;
}

char *
win32OpenFileDlg(char const *filename_hint, FileDlgFilter *filters, usize num_filters,
                 cfAllocator *alloc, u32 *out_size)
{

    WCHAR name[MAX_PATH] = {0};

    if (filename_hint)
    {
        u32 name_size = win32Utf8To16(filename_hint, -1, NULL, 0);
        if (name_size > MAX_PATH) return NULL;
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

    char *result = NULL;

    if (GetOpenFileNameW(&ofn))
    {
        u32 result_size = win32Utf16To8(ofn.lpstrFile, -1, NULL, 0);
        result = cfAlloc(alloc, result_size);

        if (result)
        {
            *out_size = result_size;
            win32Utf16To8(ofn.lpstrFile, -1, result, result_size);
        }
    }

    cfFree(alloc, filt, filt_size);

    return result;
}

//------------------------------------------------------------------------------

u32
win32Utf8To16(char const *str, i32 str_size, WCHAR *out, u32 out_size)
{
    return MultiByteToWideChar(CP_UTF8, MB_PRECOMPOSED, str, str_size, out, out_size);
}

u32
win32Utf16To8(WCHAR const *str, i32 str_size, char *out, u32 out_size)
{
    return WideCharToMultiByte(CP_UTF8, 0, str, str_size, out, out_size, 0, false);
}

//------------------------------------------------------------------------------

#pragma warning(pop)
