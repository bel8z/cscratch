#include "platform.h"

#include "foundation/core.h"

#include "foundation/error.h"
#include "foundation/fs.h"
#include "foundation/memory.h"
#include "foundation/paths.h"
#include "foundation/strings.h"
#include "foundation/time.h"

#include "foundation/math.inl"
#include "foundation/win32.inl"

//  NOTE (Matteo): Link to relevant win32 libs
#pragma comment(lib, "kernel32")
#pragma comment(lib, "user32")
#pragma comment(lib, "gdi32")
#pragma comment(lib, "mincore")

//------------------------------------------------------------------------------
// API interface
//------------------------------------------------------------------------------

//---- Cross-platform entry point ----//

I32 platformMain(Platform *platform, CommandLine *cmd_line);

//---- Virtual memory ----//

static VM_RESERVE_FUNC(win32VmReserve);
static VM_COMMIT_FUNC(win32VmCommit);
static VM_REVERT_FUNC(win32VmDecommit);
static VM_RELEASE_FUNC(win32VmRelease);

static VM_MIRROR_ALLOCATE(win32MirrorAllocate);
static VM_MIRROR_FREE(win32MirrorFree);

//---- Heap allocation ----//

static MEM_ALLOCATOR_FUNC(win32Alloc);

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

static Usize
win32GetCommandLineArgs(MemAllocator alloc, CommandLine *out)
{
    Cstr16 cmd_line = GetCommandLineW();

    I32 num_args = 0;
    Cstr16 *args16 = (Cstr16 *)CommandLineToArgvW(cmd_line, &num_args);

    if (num_args < 0)
    {
        win32HandleLastError();
        return 0;
    }

    out->len = (Usize)(num_args);

    Usize out_size = (Usize)(out->len) * sizeof(*out->arg) + CF_MB(1);

    out->arg = memAlloc(alloc, out_size);
    Cstr buf = (Char8 *)(out->arg + out->len);

    for (Usize i = 0; i < out->len; ++i)
    {
        out->arg[i] = buf;
        Str16 arg16 = str16FromCstr(args16[i]);
        Usize size = win32Utf16To8(arg16, NULL, 0);
        win32Utf16To8(arg16, (Char8 *)out->arg[i], size);
        buf += size + 1; // Ensure space for the null-terminator
    }

    LocalFree(args16);

    return out_size;
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

    // ** Init paths **

    pathsInit(g_platform.paths);

    // ** Start time tracking **

    clockStart(&g_platform.clock);
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

    CommandLine cmd_line = {0};
    Usize cmd_line_size = win32GetCommandLineArgs(g_platform.heap, &cmd_line);

    I32 result = platformMain(&g_platform, &cmd_line);

    memFree(g_platform.heap, cmd_line.arg, cmd_line_size);

    win32PlatformShutdown();

    return result;
}

I32
main(I32 argc, Cstr argv[])
{
    win32PlatformInit();
    I32 result = platformMain(&g_platform, &(CommandLine){.arg = argv, .len = (Usize)argc});
    win32PlatformShutdown();
    return result;
}

//------------------------------------------------------------------------------
// API implementation
//------------------------------------------------------------------------------

#define WIN32_PLACEHOLDER_API NTDDI_VERSION >= NTDDI_WIN10_RS4

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
        win32HandleLastError();
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
        win32HandleLastError();
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
    static_assert(sizeof(DWORD) == sizeof(buffer_size), "Unexpected pointer size");
    HANDLE mapping = CreateFileMappingW(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0,
                                        (DWORD)(buffer_size), NULL);
#endif

    if (mapping)
    {
#if WIN32_PLACEHOLDER_API
        // NOTE (Matteo): On Windows 10 use the placeholder API to double map the memory region

        HANDLE process = GetCurrentProcess();
        U8 *address = VirtualAlloc2(process, NULL, buffer_size * 2,
                                    MEM_RESERVE | MEM_RESERVE_PLACEHOLDER, PAGE_NOACCESS, NULL, 0);
        VirtualFree(address, buffer_size, MEM_RELEASE | MEM_PRESERVE_PLACEHOLDER);

        U8 *view1 = MapViewOfFile3(mapping, process, address, 0, buffer_size,
                                   MEM_REPLACE_PLACEHOLDER, PAGE_READWRITE, NULL, 0);

        U8 *view2 = MapViewOfFile3(mapping, process, address + buffer_size, 0, buffer_size,
                                   MEM_REPLACE_PLACEHOLDER, PAGE_READWRITE, NULL, 0);

        if (view1 && view2)
        {
            CF_ASSERT(view1 == address, "Logic error");
            buffer.data = view1;
        }
        else
        {
            win32HandleLastError();
        }
#else
        // NOTE (Matteo): When the placeholder API is not available, use a brute force strategy to
        // double map the region:
        // 1) try to reserve a VM region of the appropriate size, keep its address and release it
        // 2) try to map 2 views on this region; this can fail due to concurrency (since we released
        // it, the address could have been mapped by some other thread).
        // 3) repeat on failure (50 times seems reasonable)

        for (Usize try = 0; try < 50; ++try)
        {
            U8 *address = VirtualAlloc(NULL, buffer_size * 2, MEM_RESERVE, PAGE_READWRITE);
            // NOTE (Matteo): If allocation fails there's no way to map the region
            if (!address) break;

            VirtualFree(address, 0, MEM_RELEASE);

            U8 *view1 = MapViewOfFileEx(mapping, FILE_MAP_ALL_ACCESS, 0, 0, buffer_size, address);
            U8 *view2 = MapViewOfFileEx(mapping, FILE_MAP_ALL_ACCESS, 0, 0, buffer_size,
                                        view1 + buffer_size);

            if (view1 && view2)
            {
                CF_ASSERT(view1 == address, "Logic error");
                buffer.data = view1;
                break;
            }
        }
#endif

        if (buffer.data)
        {
            buffer.size = buffer_size;
            buffer.os_handle = mapping;
            memClear(buffer.data, buffer.size);
        }
        else
        {
            CloseHandle(mapping);
        }
    }

    return buffer;
}

VM_MIRROR_FREE(win32MirrorFree)
{
    if (buffer->data)
    {
        U8 *view1 = buffer->data;
        U8 *view2 = view1 + buffer->size;

#if WIN32_PLACEHOLDER_API
        HANDLE process = GetCurrentProcess();
        if (!UnmapViewOfFile2(process, view2, 0) || !UnmapViewOfFile2(process, view1, 0))
        {
            win32HandleLastError();
        }
#else
        if (!UnmapViewOfFile(view2) || UnmapViewOfFile(view1))
        {
            win32HandleLastError();
        }
#endif
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

    // TODO (Matteo): Handle alignment?
    CF_ASSERT((align & (align - 1)) == 0, "Alignment is not a power of 2");

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

    HANDLE heap = GetProcessHeap();
    void *old_mem = memory;
    void *new_mem = NULL;

    CF_ASSERT((align & (align - 1)) == 0, "Alignment is not a power of 2");
    CF_ASSERT(align < MEMORY_ALLOCATION_ALIGNMENT, "Unsupported alignment");

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
        win32HandleLastError();
        CF_ASSERT(FALSE, "LoadLibraryW FAILED");
    }

    return lib;
}

void
win32libUnload(Library *lib)
{
    if (!FreeLibrary((HMODULE)lib))
    {
        win32HandleLastError();
        CF_ASSERT(FALSE, "FreeLibrary FAILED");
    }
}

void *
win32libLoadProc(Library *lib, Cstr name)
{
    return (void *)GetProcAddress((HMODULE)lib, name);
}

//-----------------------------------------------------------------------------
