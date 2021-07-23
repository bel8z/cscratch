#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "foundation/list.h"
#include "foundation/memory.h"
#include "foundation/strings.h"

//======================================================//

// Memory allocator based on arena + free list

typedef struct MemoryHeader
{
    Usize size;
    CfList node;
} MemoryHeader;

typedef struct FreeListAlloc
{
    Arena arena;
    CfList sentinel;
} FreeListAlloc;

void
freeListAllocInit(FreeListAlloc *fl, Usize size)
{
    arenaInitOnBuffer(&fl->arena, calloc(1, size), size);
    cfListInit(&fl->sentinel);
}

MemoryHeader *
freeListAllocGetBlock(FreeListAlloc *alloc, Usize size)
{
    // NOTE (Matteo): Search the free list for a large enough block
    MemoryHeader *free_block = NULL;
    CfList *cursor = alloc->sentinel.next;

    while (cursor != &alloc->sentinel && !free_block)
    {
        MemoryHeader *block = cfListItem(cursor, MemoryHeader, node);
        if (block->size >= size)
        {
            free_block = block;
        }
    }

    if (free_block)
    {
        // NOTE (Matteo): Free block found -> to avoid wasting memory, split it if too large
        F64 fill_ratio = (F64)size / (F64)free_block->size;
        if (fill_ratio <= 0.25)
        {
            Usize total_size = free_block->size + sizeof(*free_block);
            U8 *split_pos = (U8 *)free_block + total_size / 2;
            MemoryHeader *next_block = (MemoryHeader *)split_pos;

            free_block->size = (Usize)(next_block - free_block) - sizeof(*free_block);
            next_block->size = total_size - free_block->size - sizeof(*next_block);

            cfListPushTail(&alloc->sentinel, &next_block->node);
        }

        cfListRemove(&free_block->node);
    }
    else
    {
        free_block = arenaAlloc(&alloc->arena, size + sizeof(*free_block));
        free_block->size = size;
        cfListInit(&free_block->node);
    }

    return free_block;
}

CF_ALLOCATOR_FUNC(freeListAllocProc)
{
    CF_ASSERT(align <= CF_MAX_ALIGN, "Unsupported alignment request");

    FreeListAlloc *alloc = state;
    U8 *new_memory = NULL;

    if (memory)
    {
        MemoryHeader *curr_block = (MemoryHeader *)(memory)-1;
        CF_ASSERT(old_size <= curr_block->size, "");

        if (new_size && curr_block->size >= new_size)
        {
            new_memory = memory;
        }
        else if (new_size)
        {
            MemoryHeader *free_block = freeListAllocGetBlock(alloc, new_size);
            CF_ASSERT_NOT_NULL(free_block);
            CF_ASSERT(free_block->size >= new_size, "Retrieved free block is too small");

            new_memory = (U8 *)(free_block + 1);
            cfMemCopy(memory, new_memory, new_size);
            cfMemClear(new_memory + new_size, free_block->size - new_size);
        }

        if (memory != new_memory)
        {
            // NOTE (Matteo): If the current block is not used anymore, push it on the free list
            cfListPushTail(&alloc->sentinel, &curr_block->node);
        }
    }
    else if (new_size)
    {
        MemoryHeader *free_block = freeListAllocGetBlock(alloc, new_size);
        CF_ASSERT_NOT_NULL(free_block);
        CF_ASSERT(free_block->size >= new_size, "Retrieved free block is too small");
        new_memory = (U8 *)(free_block + 1);
        cfMemClear(new_memory, new_size);
    }

    return new_memory;
}

CfAllocator
freeListAllocator(FreeListAlloc *alloc)
{
    return (CfAllocator){
        .state = alloc,
        .func = freeListAllocProc,
    };
}

//======================================================//

// Windows thread pool IO

#include "foundation/win32.h"

#define IO_CALLBACK(name) void name(void *context, ULONG result, ULONG_PTR bytes)

typedef IO_CALLBACK((*IoCallback));

typedef struct IoContext
{
    IoCallback callback;
    void *user_data;
    OVERLAPPED ovp;
} IoContext;

void WINAPI
ioCallback(TP_CALLBACK_INSTANCE *Instance,     //
           void *Context,                      //
           void *Overlapped,                   //
           ULONG IoResult,                     //
           ULONG_PTR NumberOfBytesTransferred, //
           TP_IO *Io)
{
    CF_UNUSED(Instance);

    IoContext *ioctxt = Context;
    ioctxt->callback(ioctxt->user_data, IoResult, NumberOfBytesTransferred);

    CF_ASSERT(Overlapped == &ioctxt->ovp, "");

    CloseThreadpoolIo(Io);
    free(ioctxt);
}

TP_IO *
ioBegin(IoContext *context, HANDLE file)
{
    TP_IO *io = CreateThreadpoolIo(file, ioCallback, context, NULL);
    StartThreadpoolIo(io);
    return io;
}

typedef struct FileIoToken
{
    bool completed;
    bool success;
} FileIoToken;

IO_CALLBACK(fileIoCallback)
{
    CF_UNUSED(bytes);
    FileIoToken *token = context;
    token->success = !result;
    token->completed = true;
}

void
fileBeginWrite(Cstr filename, U8 const *buffer, Usize size, FileIoToken *token)
{
    IoContext *context = calloc(1, sizeof(*context));
    context->callback = fileIoCallback;
    context->user_data = token;

    HANDLE file = CreateFileA(filename, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
                              FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED, NULL);

    TP_IO *io = ioBegin(context, file);

    token->success = WriteFile(file, buffer, (DWORD)size, NULL, &context->ovp);

    if (token->success || GetLastError() != ERROR_IO_PENDING)
    {
        token->completed = true;
        CancelThreadpoolIo(io);
    }
}

//======================================================//

#define ALLOC_SIZE CF_MB(1)
#define BUFF_SIZE 1024

int
main()
{
    FreeListAlloc fl = {0};
    freeListAllocInit(&fl, ALLOC_SIZE);
    CfAllocator alloc = freeListAllocator(&fl);

    //======================================================//

    printf("-------------------------\n");
    printf("Temporary buffer\n");
    printf("-------------------------\n");

    Char8 *buff = cfMemAlloc(alloc, BUFF_SIZE);

    strPrintf(buff, BUFF_SIZE, "USIZE_MAX = %zu", USIZE_MAX);
    Str dummy = strFromCstr(buff);
    strPrintf(buff, BUFF_SIZE, "%.*s", (I32)dummy.len, dummy.buf);
    printf("%.*s\n", (I32)dummy.len, dummy.buf);

    cfMemFree(alloc, buff, BUFF_SIZE);

    //======================================================//

    printf("-------------------------\n");
    printf("String builder\n");
    printf("-------------------------\n");

    StrBuffer sb = {0};

    strBufferInit(&sb, alloc);
    strBufferPrintf(&sb, "%s", "This is");
    strBufferAppend(&sb, strFromCstr(" a"));
    strBufferAppendf(&sb, " %s %s", "string", "buffer");
    printf("%s\n", strBufferCstr(&sb));

    strBufferShutdown(&sb);

    //======================================================//

    U8 *big_block = calloc(1, CF_GB(1));
    FileIoToken token = {0};

    fileBeginWrite("C:/Temp/BigFile.bin", big_block, CF_GB(1), &token);

    while (!token.completed)
    {
        Sleep(1);
    }

    free(big_block);
    printf("%s\n", token.success ? "SUCCESS" : "FAILURE");

    return 0;
}
