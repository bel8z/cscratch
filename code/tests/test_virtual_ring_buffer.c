#include "foundation/core.h"
#include "foundation/memory.h"

#include "foundation/win32.h"

#include "stdio.h"

// NOTE (Matteo): This buffer is made by two ajacent virtual memory regions that both map to the
// same physical memory.
// This allows for an apparently continuous layout of memory even when writes wrap around the end of
// the buffer.

typedef struct MirrorBuffer
{
    Usize size;
    U8 *data;
    void *os_handle;
} MirrorBuffer;

MirrorBuffer
mirrorAllocate(Usize size)
{
    SYSTEM_INFO info = {0};
    GetSystemInfo(&info);

    // NOTE (Matteo): Size is rounded to virtual memory granularity because the mapping addresses
    // must be aligned as such.
    Usize granularity = info.dwAllocationGranularity;
    Usize buffer_size = (size + granularity - 1) & ~(granularity - 1);

    MirrorBuffer mb = {0};

    HANDLE mapping =
        CreateFileMappingW(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, (DWORD)(buffer_size >> 32),
                           (DWORD)(buffer_size & 0xffffffff), NULL);

    if (mapping)
    {
        mb.size = buffer_size;
        mb.os_handle = mapping;

        while (!mb.data)
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
                    mb.data = view1;
                }
                else
                {
                    win32PrintLastError();
                }
            }
        }

        cfMemClear(mb.data, mb.size);
    }

    return mb;
}

void
mirrorFree(MirrorBuffer *mb)
{
    if (mb->data)
    {
        UnmapViewOfFile(mb->data + mb->size);
        UnmapViewOfFile(mb->data);
    }

    if (mb->os_handle) CloseHandle(mb->os_handle);

    mb->size = 0;
    mb->data = 0;
    mb->os_handle = 0;
}

int
main(void)
{
    MirrorBuffer mb = mirrorAllocate(1);

    Usize write_pos = 0;
    bool repeat = true;

    while (repeat)
    {
        if (write_pos > mb.size) repeat = false;

        char *ptr = (char *)mb.data + (write_pos & (mb.size - 1));
        write_pos += (Usize)sprintf(ptr, "Write pos: %zu\n", write_pos);
    }

    fprintf(stdout, "%s", (char *)mb.data);

    // Cleanup
    mirrorFree(&mb);

    return 0;
}
