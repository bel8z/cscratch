#include "platform.h"

// Interface between platform layer and hosted applicatiom
#include "app.h"

// Gui library
#include "gui/gui.h"
#include "gui/gui_backend_gl3.h"
#include "gui/win.h"

// Backend libraries
#include "gl/gl_api.h"

// Foundation library
#include "foundation/core.h"

#include "foundation/colors.h"
#include "foundation/error.h"
#include "foundation/io.h"
#include "foundation/memory.h"
#include "foundation/paths.h"
#include "foundation/strings.h"

#include "foundation/math.inl"
#include "foundation/win32.inl"

#define WIN32_PLACEHOLDER_API NTDDI_VERSION >= NTDDI_WIN10_RS4

#if WIN32_PLACEHOLDER_API
#    pragma comment(lib, "mincore")
#endif

//------------------------------------------------------------------------------
// Platform API
//------------------------------------------------------------------------------

//---- Cross-platform entry point for console applications ----//

I32 consoleMain(Platform *platform, CommandLine *cmd_line);

//---- Virtual memory ----//

CF_INTERNAL VMEM_RESERVE_FN(win32VmReserve);
CF_INTERNAL VMEM_COMMIT_FN(win32VmCommit);
CF_INTERNAL VMEM_DECOMMIT_FN(win32VmDecommit);
CF_INTERNAL VMEM_RELEASE_FN(win32VmRelease);

CF_INTERNAL VMEM_MIRROR_ALLOCATE_FN(win32MirrorAllocate);
CF_INTERNAL VMEM_MIRROR_FREE_FN(win32MirrorFree);

//---- Heap allocation ----//

CF_INTERNAL MEM_ALLOCATOR_FN(win32Alloc);

//---- File system ----//

CF_INTERNAL IO_FILE_COPY(win32FileCopy);
CF_INTERNAL IO_FILE_OPEN(win32FileOpen);
CF_INTERNAL IO_FILE_CLOSE(win32FileClose);
CF_INTERNAL IO_FILE_SIZE(win32FileSize);
CF_INTERNAL IO_FILE_SEEK(win32FileSeek);
CF_INTERNAL IO_FILE_READ(win32FileRead);
CF_INTERNAL IO_FILE_READ_AT(win32FileReadAt);
CF_INTERNAL IO_FILE_WRITE(win32FileWrite);
CF_INTERNAL IO_FILE_WRITE_AT(win32FileWriteAt);
CF_INTERNAL IO_FILE_PROPERTIES(win32FileProperties);
CF_INTERNAL IO_FILE_PROPERTIES_P(win32FilePropertiesP);

CF_INTERNAL IO_DIRECTORY_OPEN(win32DirectoryOpen);

//---- Global platform API ----//

// NOTE (Matteo): a global here should be quite safe
CF_GLOBAL Platform g_platform = {
    .vmem =
        &(VMemApi){
            .reserve = win32VmReserve,
            .release = win32VmRelease,
            .commit = win32VmCommit,
            .decommit = win32VmDecommit,
            .mirrorAllocate = win32MirrorAllocate,
            .mirrorFree = win32MirrorFree,
        },
    .file =
        &(IoFileApi){
            .invalid = INVALID_HANDLE_VALUE,
            .std_in = INVALID_HANDLE_VALUE,
            .std_out = INVALID_HANDLE_VALUE,
            .std_err = INVALID_HANDLE_VALUE,
            .copy = win32FileCopy,
            .open = win32FileOpen,
            .close = win32FileClose,
            .size = win32FileSize,
            .properties = win32FileProperties,
            .propertiesP = win32FilePropertiesP,
            .seek = win32FileSeek,
            .read = win32FileRead,
            .readAt = win32FileReadAt,
            .write = win32FileWrite,
            .writeAt = win32FileWriteAt,
            .dirOpen = win32DirectoryOpen,
        },
    .paths = &(Paths){0},
};

//------------------------------------------------------------------------------
// Application API
//------------------------------------------------------------------------------

typedef struct AppApi
{
    void *lib;

    AppCreateProc create;
    AppProc destroy;

    AppProc load;
    AppProc unload;

    AppUpdateProc update;

    // TODO (Matteo): reduce redundancy
    Char8 src_file[Paths_Size];
    Char8 dst_file[Paths_Size];
} AppApi;

CF_INTERNAL APP_FN(appProcStub);
CF_INTERNAL APP_CREATE_FN(appCreateStub);
CF_INTERNAL APP_UPDATE_FN(appUpdateStub);

CF_INTERNAL void appApiLoad(AppApi *api, Paths *paths);
CF_INTERNAL void appApiUpdate(AppApi *api, Paths *paths, AppState *app);

//------------------------------------------------------------------------------
// Utilities
//------------------------------------------------------------------------------

CF_INTERNAL Usize
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

    LocalFree((Char16 *)args16);

    return out_size;
}

CF_INTERNAL void *
win32libLoad(Str filename)
{
    Char16 buffer[1024] = {0};
    win32Utf8To16(filename, buffer, CF_ARRAY_SIZE(buffer));
    void *lib = LoadLibraryW(buffer);

    if (!lib)
    {
        win32HandleLastError();
        CF_ASSERT(false, "LoadLibraryW FAILED");
    }

    return lib;
}

CF_INTERNAL void
win32libUnload(void *lib)
{
    if (!FreeLibrary((HMODULE)lib))
    {
        win32HandleLastError();
        CF_ASSERT(false, "FreeLibrary FAILED");
    }
}

CF_INTERNAL void *
win32libLoadProc(void *lib, Cstr name)
{
    return (void *)GetProcAddress((HMODULE)lib, name);
}

//------------------------------------------------------------------------------
// Main entry point
//------------------------------------------------------------------------------

CF_INTERNAL void
win32PathsInit(Paths *g_paths)
{
    // Clear shared buffer
    memClear(g_paths->buffer, CF_ARRAY_SIZE(g_paths->buffer));

    // Point string views to assigned positions
    g_paths->base.ptr = g_paths->buffer;
    g_paths->lib_name.ptr = g_paths->buffer + 1 * Paths_Size;
    g_paths->data.ptr = g_paths->buffer + 2 * Paths_Size;

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
    strPrint((Char8 *)g_paths->lib_name.ptr, Paths_Size, "%.*s%s",
             (I32)(g_paths->exe_name.len - ext.len), g_paths->exe_name.ptr, "_lib.dll");

    g_paths->lib_name.len = strLength(g_paths->lib_name.ptr);

    // Build data path from base path
    strPrint((Char8 *)g_paths->data.ptr, Paths_Size, "%.*sdata\\", (I32)g_paths->base.len,
             g_paths->base.ptr);
    g_paths->data.len = strLength(g_paths->data.ptr);
}

CF_INTERNAL void
win32PlatformInit(void)
{
    // ** Init memory management **

    SYSTEM_INFO sysinfo;
    GetSystemInfo(&sysinfo);

    g_platform.vmem->page_size = sysinfo.dwPageSize;
    g_platform.vmem->address_granularity = sysinfo.dwAllocationGranularity;

#if CF_MEMORY_PROTECTION
    CF_UNUSED(win32Alloc);
    g_platform.heap = memEndOfPageAllocator(g_platform.vmem);
#else
    g_platform.heap.state = &g_platform;
    g_platform.heap.func = win32Alloc;
#endif

    // ** Init IO pipes **

    g_platform.file->std_in = GetStdHandle(STD_INPUT_HANDLE);
    g_platform.file->std_out = GetStdHandle(STD_OUTPUT_HANDLE);
    g_platform.file->std_err = GetStdHandle(STD_ERROR_HANDLE);

    // ** Init paths **

    win32PathsInit(g_platform.paths);

    // ** Start time tracking **

    clockStart(&g_platform.clock);
}

CF_INTERNAL void
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
    Paths *paths = g_platform.paths;

    // Setup platform layer
    clockStart(&g_platform.clock);

    // NOTE (Matteo): Custom IMGUI ini file
    // TODO (Matteo): Clean up!

    CF_DIAGNOSTIC_PUSH()
    CF_DIAGNOSTIC_IGNORE_MSVC(4221)

    Char8 gui_ini[Paths_Size] = {0};
    CF_ASSERT(paths->base.len + paths->exe_name.len < Paths_Size, "IMGUI ini file name too long");
    memCopy(paths->base.ptr, gui_ini, paths->base.len);
    memCopy(paths->exe_name.ptr, gui_ini + paths->base.len, paths->exe_name.len);
    pathChangeExt(strFromCstr(gui_ini), strLiteral(".gui"), gui_ini);

    // Setup Dear ImGui context
    GuiOpenGLVersion gl_ver = {0};
    g_platform.gui = guiInit(
        &(GuiInitInfo){
            .alloc = g_platform.heap,
            .ini_filename = gui_ini,
            .data_path = paths->data,
            .gl_context = true,
        },
        &gl_ver);

    if (!g_platform.gui) return -1;

    CF_DIAGNOSTIC_POP()

    // Initialize OpenGL loader
    if (!glApiLoad(guiLoadProc) || !glApiIsSupported(gl_ver.major, gl_ver.minor))
    {
        CF_LOG("Failed to initialize OpenGL loader!\n");
        return -2;
    }
    CF_ASSERT_NOT_NULL(gl);
    g_platform.gl = gl;

    // Setup application
    AppApi app_api = {0};
    appApiLoad(&app_api, g_platform.paths);
    AppState *app_state = app_api.create(&g_platform, &cmd_line);

    // Setup Platform/Renderer backends
    guiGl3Init(gl_ver);

    // Main loop
    AppIo app_io = {
        .font_opts = &(GuiFontOptions){.rasterizer_multiply = 1.0f},
        // NOTE (Matteo): Ensure font rebuild before first frame
        .rebuild_fonts = true,
        // NOTE (Matteo): Ensure fast first update
        .continuous_update = true,
    };

    IVec2 display = {0};
    while (guiEventLoop(!app_io.continuous_update, app_io.fullscreen, &display) && !app_io.quit)
    {
        //------------------//
        // Event processing //
        //------------------//

        // TODO (Matteo): Maybe improve a bit?
        if (app_io.window_title_changed)
        {
            guiSetTitle(app_io.window_title);
        }

        // NOTE (Matteo): Auto reloading of application library
        appApiUpdate(&app_api, paths, app_state);

        // Rebuild font atlas if required
        if (app_io.rebuild_fonts)
        {
            app_io.rebuild_fonts = false;
            guiUpdateAtlas(guiFonts(), app_io.font_opts);
            // Re-upload font texture on the GPU
            guiGl3UpdateFontsTexture();
        }

        //--------------//
        // Frame update //
        //--------------//

        // Start the Dear ImGui frame
        guiGl3NewFrame();
        guiNewFrame();

        // NOTE (Matteo): Setup GL viewport and clear buffers BEFORE app update in order to allow
        // the application code to draw directly using OpenGL
        LinearColor clear_color = colorToLinearMultiplied(app_io.back_color);

        glViewport(0, 0, display.width, display.height);
        glClearColor(clear_color.r, clear_color.g, clear_color.b, clear_color.a);
        glClear(GL_COLOR_BUFFER_BIT);

        // Application frame update
        app_api.update(app_state, &app_io);

        //-----------//
        // Rendering //
        //-----------//

        GuiDrawData *draw_data = guiRender();
        guiGl3Render(draw_data);
        guiUpdateViewports(true);
    }

    // Cleanup
    guiGl3Shutdown();
    guiShutdown(g_platform.gui);
    app_api.destroy(app_state);

    memFree(g_platform.heap, (Char8 *)cmd_line.arg, cmd_line_size);

    win32PlatformShutdown();

    return 0;
}

I32
main(I32 argc, Cstr argv[])
{
    win32PlatformInit();
    I32 result = consoleMain(&g_platform, &(CommandLine){.arg = argv, .len = (Usize)argc});
    win32PlatformShutdown();
    return result;
}

//------------------------------------------------------------------------------
// API implementation
//------------------------------------------------------------------------------

//------------//
//   Memory   //
//------------//

VMEM_RESERVE_FN(win32VmReserve)
{
    void *mem = VirtualAlloc(NULL, size, MEM_RESERVE, PAGE_NOACCESS);

    if (mem)
    {
        g_platform.reserved_size += size;
    }

    return mem;
}

VMEM_COMMIT_FN(win32VmCommit)
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

VMEM_DECOMMIT_FN(win32VmDecommit)
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

VMEM_RELEASE_FN(win32VmRelease)
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

VMEM_MIRROR_ALLOCATE_FN(win32MirrorAllocate)
{
    // NOTE (Matteo): Size is rounded to virtual memory granularity because the mapping addresses
    // must be aligned as such.
    Usize granularity = g_platform.vmem->address_granularity;
    Usize buffer_size = (size + granularity - 1) & ~(granularity - 1);

    VMemMirrorBuffer buffer = {0};

#if (CF_PTR_SIZE == 8)
    HANDLE mapping =
        CreateFileMappingW(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, (DWORD)(buffer_size >> 32),
                           (DWORD)(buffer_size & 0xffffffff), NULL);
#else
    CF_STATIC_ASSERT(sizeof(DWORD) == sizeof(buffer_size), "Unexpected pointer size");
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

VMEM_MIRROR_FREE_FN(win32MirrorFree)
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

MEM_ALLOCATOR_FN(win32Alloc)
{
    CF_UNUSED(state);

    HANDLE heap = GetProcessHeap();
    void *old_mem = memory;
    void *new_mem = NULL;

    CF_ASSERT(cfIsPowerOf2(align), "Alignment is not a power of 2");
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

//-----------------//
//   File system   //
//-----------------//

// TODO (Matteo): Provide async file IO? Win32 offers IO Completion Ports which seem very good for
// the purpose

typedef struct Win32DirIterator
{
    HANDLE finder;
    WIN32_FIND_DATAW data;
    Char8 buffer[sizeof(IoDirectory) - sizeof(HANDLE) - sizeof(WIN32_FIND_DATAW)];
} Win32DirIterator;

// NOTE (Matteo): Ensure that there is room for a reasonably sized buffer
CF_STATIC_ASSERT(sizeof(((Win32DirIterator *)0)->buffer) >= 512,
                 "FsIterator buffer size is too small");

CF_INTERNAL inline U64
win32MergeWords(DWORD high, DWORD low)
{
    ULARGE_INTEGER value = {.HighPart = high, .LowPart = low};
    return value.QuadPart;
}

CF_INTERNAL inline SystemTime
win32FileSystemTime(FILETIME time)
{
    return win32MergeWords(time.dwHighDateTime, time.dwLowDateTime);
}

CF_INTERNAL void
win32ReadAttributes(DWORD attributes, IoFileProperties *out)
{
    if (attributes & FILE_ATTRIBUTE_DIRECTORY)
    {
        out->attributes |= IoFileAttributes_Directory;
    }

    if (attributes & FILE_ATTRIBUTE_REPARSE_POINT)
    {
        out->attributes |= IoFileAttributes_Symlink;
    }
}

CF_INTERNAL OVERLAPPED
win32FileOffset(Usize offset)
{
    ULARGE_INTEGER temp_offset = {.QuadPart = offset};
    OVERLAPPED overlapped = {.Offset = temp_offset.LowPart, .OffsetHigh = temp_offset.HighPart};
    return overlapped;
}

IO_FILE_COPY(win32FileCopy)
{
    Char16 ws[MAX_PATH] = {0};
    Char16 wd[MAX_PATH] = {0};
    win32Utf8To16(source, ws, CF_ARRAY_SIZE(ws));
    win32Utf8To16(dest, wd, CF_ARRAY_SIZE(wd));

    if (CopyFileW(ws, wd, !overwrite)) return true;

    win32HandleLastError();
    return false;
}

IO_FILE_OPEN(win32FileOpen)
{
    IoFile *result = INVALID_HANDLE_VALUE;

    if (mode != 0)
    {
        DWORD access = 0;
        DWORD creation = OPEN_EXISTING;

        if (mode & IoOpenMode_Read) access |= GENERIC_READ;

        if (mode & IoOpenMode_Write)
        {
            access |= GENERIC_WRITE;
            // Overwrite creation mode
            creation = CREATE_ALWAYS;
        }

        Char16 buffer[1024] = {0};
        win32Utf8To16(filename, buffer, CF_ARRAY_SIZE(buffer));

        result = CreateFileW(buffer, access, FILE_SHARE_READ, NULL, creation, FILE_ATTRIBUTE_NORMAL,
                             NULL);

        if (result == INVALID_HANDLE_VALUE)
        {
            win32HandleLastError();
        }
        else if (mode == IoOpenMode_Append)
        {
            if (!SetFilePointer(result, 0, NULL, FILE_END))
            {
                CloseHandle(result);
                result = INVALID_HANDLE_VALUE;
                win32HandleLastError();
            }
        }
    }

    return result;
}

IO_FILE_CLOSE(win32FileClose)
{
    CloseHandle(file);
}

IO_FILE_SIZE(win32FileSize)
{
    if (file == INVALID_HANDLE_VALUE) return USIZE_MAX;

    ULARGE_INTEGER size;

    size.LowPart = GetFileSize(file, &size.HighPart);

    return (Usize)size.QuadPart;
}

IO_FILE_PROPERTIES(win32FileProperties)
{
    IoFileProperties props = {0};
    BY_HANDLE_FILE_INFORMATION info;

    if (GetFileInformationByHandle(file, &info))
    {
        props.exists = true;
        props.last_write = win32FileSystemTime(info.ftLastWriteTime);
        props.size = (Usize)win32MergeWords(info.nFileSizeHigh, info.nFileSizeLow);
        win32ReadAttributes(info.dwFileAttributes, &props);
    }

    return props;
}

IO_FILE_PROPERTIES_P(win32FilePropertiesP)
{
    IoFileProperties props = {0};

    Char16 wide_name[MAX_PATH] = {0};
    win32Utf8To16(path, wide_name, CF_ARRAY_SIZE(wide_name));

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

IO_FILE_SEEK(win32FileSeek)
{
    if (file == INVALID_HANDLE_VALUE) return USIZE_MAX;

    LARGE_INTEGER temp = {.QuadPart = offset};
    LARGE_INTEGER dest = {0};

    CF_ASSERT(offset > 0 || pos == IoSeekPos_Current,
              "Negative offset is supported only if seeking from the current position");

    if (!SetFilePointerEx(file, temp, &dest, pos))
    {
        win32HandleLastError();
        return USIZE_MAX;
    };

    return (Usize)dest.QuadPart;
}

IO_FILE_READ(win32FileRead)
{
    if (file == INVALID_HANDLE_VALUE) return USIZE_MAX;

    DWORD read_bytes;

    if (!ReadFile(file, buffer, (DWORD)buffer_size, &read_bytes, NULL))
    {
        win32HandleLastError();
        return USIZE_MAX;
    }

    return read_bytes;
}

IO_FILE_READ_AT(win32FileReadAt)
{
    if (file == INVALID_HANDLE_VALUE) return USIZE_MAX;

    OVERLAPPED overlapped = win32FileOffset(offset);
    DWORD read_bytes;

    if (!ReadFile(file, buffer, (DWORD)buffer_size, &read_bytes, &overlapped))
    {
        win32HandleLastError();
        return USIZE_MAX;
    }

    return read_bytes;
}

IO_FILE_WRITE(win32FileWrite)
{
    if (file == INVALID_HANDLE_VALUE) return USIZE_MAX;

    DWORD written_bytes;

    if (!WriteFile(file, data, (DWORD)data_size, &written_bytes, NULL))
    {
        win32HandleLastError();
        return false;
    }

    CF_ASSERT(written_bytes == (DWORD)data_size, "Incorrect number of bytes written");

    return true;
}

IO_FILE_WRITE_AT(win32FileWriteAt)
{
    if (file == INVALID_HANDLE_VALUE) return USIZE_MAX;

    OVERLAPPED overlapped = win32FileOffset(offset);
    DWORD written_bytes;

    if (!WriteFile(file, data, (DWORD)data_size, &written_bytes, &overlapped))
    {
        win32HandleLastError();
        return false;
    }

    CF_ASSERT(written_bytes == (DWORD)data_size, "Incorrect number of bytes written");

    return true;
}

CF_INTERNAL IO_DIRECTORY_NEXT(win32DirectoryNext)
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

    filename->ptr = iter->buffer;
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

CF_INTERNAL IO_DIRECTORY_CLOSE(win32DirectoryClose)
{
    CF_ASSERT_NOT_NULL(self);

    Win32DirIterator *iter = (Win32DirIterator *)self->opaque;
    FindClose(iter->finder);
}

IO_DIRECTORY_OPEN(win32DirectoryOpen)
{
    CF_ASSERT_NOT_NULL(self);

    Win32DirIterator *iter = (Win32DirIterator *)self->opaque;
    Char16 buffer[1024];

    // Encode path to UTF16
    Usize length = win32Utf8To16(path, buffer, CF_ARRAY_SIZE(buffer));

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

    self->next = win32DirectoryNext;
    self->close = win32DirectoryClose;

    return true;
}

// NOTE (Matteo): POSIX-like example
#if 0
#    include <sys/stat.h>

IoFileProperties
posixFileProperties(Str filename)
{
    IoFileProperties props = {0};

    Char8 buffer[1024] = {0};
    struct _stat64i32 info = {0};

    memCopy(filename.ptr, buffer, filename.len);
    if (!_stat(buffer, &info))
    {
        props.exists = true;
        props.last_write = (SystemTime)info.st_mtime;
        if (info.st_mode & 0x0120000) props.attributes |= IoFileAttributes_Symlink;
        if (info.st_mode & 0x0040000) props.attributes |= IoFileAttributes_Directory;
    }

    return props;
}

#endif

//------------------------------//
//   Application API handling   //
//------------------------------//

APP_FN(appProcStub)
{
    CF_UNUSED(app);
}

APP_CREATE_FN(appCreateStub)
{
    CF_UNUSED(plat);
    CF_UNUSED(cmd_line);
    return NULL;
}

APP_UPDATE_FN(appUpdateStub)
{
    CF_UNUSED(state);
    io->quit = true;
}

void
appApiLoad(AppApi *api, Paths *paths)
{
    if (api->lib)
    {
        CF_ASSERT(api->create, "");
        win32libUnload(api->lib);
        memClearStruct(api);
    }

    strPrint(api->src_file, Paths_Size, "%.*s%.*s", //
             (I32)paths->base.len, paths->base.ptr, //
             (I32)paths->lib_name.len, paths->lib_name.ptr);
    strPrint(api->dst_file, Paths_Size, "%s.tmp", api->src_file);

    Str dst_file = strFromCstr(api->dst_file);
    if (win32FileCopy(strFromCstr(api->src_file), dst_file, true))
    {
        api->lib = win32libLoad(dst_file);
    }

    if (api->lib)
    {
        api->create = (AppCreateProc)win32libLoadProc(api->lib, "appCreate");
        api->destroy = (AppProc)win32libLoadProc(api->lib, "appDestroy");
        api->load = (AppProc)win32libLoadProc(api->lib, "appLoad");
        api->unload = (AppProc)win32libLoadProc(api->lib, "appUnload");
        api->update = (AppUpdateProc)win32libLoadProc(api->lib, "appUpdate");
    }

    if (!api->create) api->create = appCreateStub;
    if (!api->destroy) api->destroy = appProcStub;
    if (!api->load) api->load = appProcStub;
    if (!api->unload) api->unload = appProcStub;
    if (!api->update) api->update = appUpdateStub;
}

void
appApiUpdate(AppApi *api, Paths *paths, AppState *app)
{
    // TODO (Matteo): Are these operation too expensive to be performed every frame?
    if (win32FilePropertiesP(strFromCstr(api->src_file)).last_write >
        win32FilePropertiesP(strFromCstr(api->dst_file)).last_write)
    {
        api->unload(app);
        appApiLoad(api, paths);
        api->load(app);
    }
}

//------------------------------------------------------------------------------
