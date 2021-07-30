#include "foundation/core.h"
#include "foundation/memory.h"

#include "foundation/win32.h"

#include "stdio.h"

// NOTE (Matteo): This buffer is made by two ajacent virtual memory regions that both map to the
// same physical memory.
// This allows for an apparently continuous layout of memory even when writes wrap around the end of
// the buffer.

typedef struct VRingBuffer
{
    HANDLE mapping;
    Usize size;
    U8 *data;
} VRingBuffer;

VRingBuffer
vringAllocate(Usize size)
{
    SYSTEM_INFO info = {0};
    GetSystemInfo(&info);

    // NOTE (Matteo): Size is rounded to virtual memory granularity because the mapping addresses
    // must be aligned as such.
    Usize granularity = info.dwAllocationGranularity;
    Usize buffer_size = (size + granularity - 1) & ~(granularity - 1);

    VRingBuffer mb = {0};

    HANDLE mapping =
        CreateFileMappingW(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, (DWORD)(buffer_size >> 32),
                           (DWORD)(buffer_size & 0xffffffff), NULL);

    if (mapping)
    {
        mb.size = buffer_size;
        mb.mapping = mapping;

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

        MapViewOfFileEx(mapping, FILE_MAP_ALL_ACCESS, 0, 0, buffer_size, mb.data + buffer_size);
    }

    return mb;
}

void
vringFree(VRingBuffer *mb)
{
    if (mb->data)
    {
        UnmapViewOfFile(mb->data + mb->size);
        UnmapViewOfFile(mb->data);
    }

    if (mb->mapping)
    {
        CloseHandle(mb->mapping);
    }

    mb->data = 0;
    mb->mapping = 0;
    mb->size = 0;
}

int
main(void)
{
    VRingBuffer mb = vringAllocate(7539);

    cfMemClear(mb.data, mb.size);
    char *s1 = (char *)mb.data;
    char *s2 = s1 + mb.size;
    sprintf(s1, "%s", "Hello, world!");

    printf("%s/n", s1);
    printf("%s/n", s2);

    fflush(stdout);

    // Cleanup
    vringFree(&mb);

    return 0;
}
