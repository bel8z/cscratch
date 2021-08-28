#include "fs.h"
#include "memory.h"

FileContent
fileReadContent(CfFileSystem *fs, Str filename, MemAllocator alloc)
{
    FileContent result = {0};

    FileHandle file = fs->fileOpen(filename, FileOpenMode_Read);

    if (!file.error)
    {
        Usize file_size = fs->fileSize(file);
        Usize read_size = file_size;

        result.data = memAlloc(alloc, read_size);

        if (result.data && fs->fileRead(file, result.data, read_size) == read_size)
        {
            result.size = read_size;
        }
        else
        {
            cfMemFree(alloc, result.data, read_size);
            result.data = NULL;
        }

        fs->fileClose(file);
    }

    return result;
}
