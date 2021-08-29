#include "fs.h"
#include "memory.h"

FileContent
fileReadContent(CfFileSystem *fs, Str filename, MemAllocator alloc)
{
    FileContent result = {0};

    FileStream file = fs->fileStreamOpen(filename, FileOpenMode_Read);

    if (!(file.flags & FileStreamFlags_Error))
    {
        Usize file_size = fs->fileStreamSize(&file);
        Usize read_size = file_size;

        result.data = memAlloc(alloc, read_size);

        if (result.data && fs->fileStreamRead(&file, result.data, read_size) == read_size)
        {
            result.size = read_size;
        }
        else
        {
            memFree(alloc, result.data, read_size);
            result.data = NULL;
        }

        fs->fileStreamClose(&file);
    }

    return result;
}
