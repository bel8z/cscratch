#include "platform.h"

#include "foundation/core.h"
#include "foundation/error.h"
#include "foundation/fs.h"
#include "foundation/math.inl"
#include "foundation/memory.h"
#include "foundation/paths.h"
#include "foundation/strings.h"
#include "foundation/time.h"
#include "foundation/win32.h"

//------------------------------------------------------------------------------
// API interface
//------------------------------------------------------------------------------

//---- Cross-platform entry point ----//

I32 platformMain(Platform *platform, Cstr argv[], I32 argc);

//---- Virtual memory ----//

static VM_RESERVE_FUNC(win32VmReserve);
static VM_COMMIT_FUNC(win32VmCommit);
static VM_REVERT_FUNC(win32VmDecommit);
static VM_RELEASE_FUNC(win32VmRelease);

static VM_MIRROR_ALLOCATE(win32MirrorAllocate);
static VM_MIRROR_FREE(win32MirrorFree);

//---- Heap allocation ----//

static MEM_ALLOCATOR_FUNC(win32Alloc);

//---- File system ----//

// TODO (Matteo): Provide async file IO? Win32 offers IO Completion Ports which seem very good for
// the purpose

// NOTE (Matteo): Ensure that there is room for a reasonably sized buffer
static_assert(sizeof(DirIterator) > 512 + sizeof(HANDLE) + sizeof(WIN32_FIND_DATAW),
              "DirIterator buffer size is too small");

//---- Timing ----//

static struct
{
    U64 start_ticks;
    U64 freq;
} g_clock;

static Duration win32Clock(void);
static SystemTime win32SystemTime(void);
static CalendarTime win32UtcTime(SystemTime sys_time);
static CalendarTime win32LocalTime(SystemTime sys_time);

//---- Dynamic loading ----//

static Library *win32libLoad(Str filename);
static void win32libUnload(Library *lib);
static void *win32libLoadProc(Library *restrict lib, Cstr restrict name);

//---- Global platform API ----//

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
    .time =
        &(CfTimeApi){
            .clock = win32Clock,
            .systemTime = win32SystemTime,
            .utcTime = win32UtcTime,
            .localTime = win32LocalTime,
        },
    .library =
        &(LibraryApi){
            .load = win32libLoad,
            .unload = win32libUnload,
            .loadSymbol = win32libLoadProc,
        },
    .paths = &(Paths){0},
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
        Str16 arg16 = str16FromCstr(argv_utf16[i]);
        Usize size = win32Utf16To8(arg16, NULL, 0);
        win32Utf16To8(arg16, argv[i], size);
        buf += size + 1; // Ensure space for the null-terminator
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

static void
win32PlatformInit(void)
{
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
    g_clock.start_ticks = (U64)now.QuadPart;
    g_clock.freq = (U64)freq.QuadPart;

    // ** Init paths **

    pathsInit(g_platform.paths);
}

static void
win32PlatformShutdown(void)
{
    CF_ASSERT(g_platform.heap_blocks == 0, "Potential memory leak");
    CF_ASSERT(g_platform.heap_size == 0, "Potential memory leak");
}

int WINAPI
wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR pCmdLine, int nCmdShow)
{
    CF_UNUSED(hInstance);
    CF_UNUSED(hPrevInstance);
    CF_UNUSED(pCmdLine);
    CF_UNUSED(nCmdShow);

    win32PlatformInit();

    // ** Platform-agnostic entry point **

    // TODO (Matteo): Improve command line handling

    Usize cmd_line_size;
    I32 argc;
    Char8 **argv = win32GetCommandLineArgs(g_platform.heap, &argc, &cmd_line_size);

    I32 result = platformMain(&g_platform, (Cstr *)argv, argc);

    memFree(g_platform.heap, argv, cmd_line_size);

    win32PlatformShutdown();

    return result;
}

I32
main(I32 argc, Cstr argv[])
{
    win32PlatformInit();
    I32 result = platformMain(&g_platform, argv, argc);
    win32PlatformShutdown();
    return result;
}

//------------------------------------------------------------------------------
// API implementation
//------------------------------------------------------------------------------

//------------//
//   Memory   //
//------------//

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

#if (CF_PTR_SIZE == 8)
    HANDLE mapping =
        CreateFileMappingW(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, (DWORD)(buffer_size >> 32),
                           (DWORD)(buffer_size & 0xffffffff), NULL);
#else
    HANDLE mapping = CreateFileMappingW(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0,
                                        (DWORD)(buffer_size & 0xffffffff), NULL);
#endif

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

//------------//
//   Timing   //
//------------//

Duration
win32Clock(void)
{
    LARGE_INTEGER now;
    QueryPerformanceCounter(&now);

    CF_ASSERT(now.QuadPart >= 0, "QueryPerformanceCounter returned negative ticks");

    U64 curr_ticks = (U64)(now.QuadPart);
    CF_ASSERT(curr_ticks > g_clock.start_ticks, "QueryPerformanceCounter wrapped around");

    U64 nanos = mMulDiv((curr_ticks - g_clock.start_ticks), CF_NS_PER_SEC, g_clock.freq);

    return timeDurationNs(nanos);
}

SystemTime
win32SystemTime(void)
{
    FILETIME time;
    GetSystemTimePreciseAsFileTime(&time);

    ULARGE_INTEGER temp = {.HighPart = time.dwHighDateTime, //
                           .LowPart = time.dwLowDateTime};
    return (U64)temp.QuadPart;
}

static CalendarTime
win32CalendarTime(SYSTEMTIME const *out)
{
    return (CalendarTime){.year = out->wYear,
                          .month = (U8)out->wMonth,
                          .day = (U8)out->wDay,
                          .week_day = (U8)out->wDayOfWeek,
                          .hour = (U8)out->wHour,
                          .minute = (U8)out->wMinute,
                          .second = (U8)out->wSecond,
                          .milliseconds = out->wMilliseconds};
}

CalendarTime
win32UtcTime(SystemTime sys_time)
{
    ULARGE_INTEGER temp = {.QuadPart = sys_time};
    FILETIME in = {.dwLowDateTime = temp.LowPart, //
                   .dwHighDateTime = temp.HighPart};
    SYSTEMTIME out;

    if (!FileTimeToSystemTime(&in, &out))
    {
        win32PrintLastError();
    }

    return win32CalendarTime(&out);
}

CalendarTime
win32LocalTime(SystemTime sys_time)
{
    // File time conversions according to MS docs:
    // https://docs.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-filetimetolocalfiletime

    ULARGE_INTEGER temp = {.QuadPart = sys_time};
    FILETIME in = {.dwLowDateTime = temp.LowPart, //
                   .dwHighDateTime = temp.HighPart};
    SYSTEMTIME utc, local;

    if (!FileTimeToSystemTime(&in, &utc) || !SystemTimeToTzSpecificLocalTime(NULL, &utc, &local))
    {
        win32PrintLastError();
    }

    return win32CalendarTime(&local);
}

//-----------------------//
//   Dynamic libraries   //
//-----------------------//

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
