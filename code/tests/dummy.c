#include "foundation/core.h"
#include "platform.h"

#include "foundation/error.h"
#include "foundation/io.h"
#include "foundation/list.h"
#include "foundation/memory.h"
#include "foundation/strings.h"

CF_GLOBAL MemAllocator g_heap;

//======================================================//

// Memory allocator based on arena + free list

typedef struct MemoryHeader
{
    Usize size;
    CfList node;
} MemoryHeader;

typedef struct FreeListAlloc
{
    MemArena arena;
    CfList sentinel;
} FreeListAlloc;

void
freeListAllocInit(FreeListAlloc *fl, void *buffer, Usize buffer_size)
{
    memArenaInitOnBuffer(&fl->arena, buffer, buffer_size);
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
            // Compute the beginning and end addresses of the block, and its midpoint
            Uptr beg = (Uptr)free_block;
            Uptr end = beg + free_block->size + sizeof(*free_block);
            Uptr mid = (beg + end) / 2;

            // Place the next block at the midpoint
            MemoryHeader *next_block = (MemoryHeader *)mid;

            // Recompute the block sizes accounting for the header sizes
            free_block->size = mid - beg - sizeof(*free_block);
            next_block->size = end - mid - sizeof(*next_block);

            cfListPushTail(&alloc->sentinel, &next_block->node);
        }

        cfListRemove(&free_block->node);
    }
    else
    {
        free_block = memArenaAlloc(&alloc->arena, size + sizeof(*free_block));
        free_block->size = size;
        cfListInit(&free_block->node);
    }

    return free_block;
}

MEM_ALLOCATOR_FN(freeListAllocProc)
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
            memCopy(memory, new_memory, new_size);
            memClear(new_memory + new_size, free_block->size - new_size);
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
        memClear(new_memory, new_size);
    }

    return new_memory;
}

MemAllocator
freeListAllocator(FreeListAlloc *alloc)
{
    return (MemAllocator){
        .state = alloc,
        .func = freeListAllocProc,
    };
}

//======================================================//

// Windows thread pool IO

#include "foundation/win32.inl"

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
    memFreeStruct(g_heap, ioctxt);
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
    IoContext *context = memAllocStruct(g_heap, IoContext);
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

typedef union F32Bits
{
    U32 u32;
    F32 f32;
} F32Bits;

typedef union F64Bits
{
    U64 u64;
    F64 f64;
} F64Bits;

I32
platformMain(Platform *platform, CommandLine *cmd_line)
{
    CF_UNUSED(cmd_line);

    g_heap = platform->heap;

    //======================================================//

    F64Bits u64 = {.f64 = 2.0};
    F32Bits u32 = {.f32 = 2.0};

    printf("%u ->\t%f\n", u32.u32, (F64)u32.f32);
    printf("%llu ->\t%f\n", u64.u64, u64.f64);

    u64.u64 = 3203822394;
    u32.u32 = 3203822394;

    printf("%u ->\t%f\n", u32.u32, (F64)u32.f32);
    printf("%llu ->\t%f\n", u64.u64, u64.f64);

    u64.f64 = 1;
    u32.f32 = 1;

    printf("%u ->\t%f\n", u32.u32, (F64)u32.f32);
    printf("%llu ->\t%f\n", u64.u64, u64.f64);

    //======================================================//

    FreeListAlloc fl = {0};
    U8 *fl_buffer = memAlloc(g_heap, ALLOC_SIZE);
    freeListAllocInit(&fl, fl_buffer, ALLOC_SIZE);
    MemAllocator alloc = freeListAllocator(&fl);

    printf("-------------------------\n");
    printf("Temporary buffer\n");
    printf("-------------------------\n");

    Char8 *buff = memAlloc(alloc, BUFF_SIZE);

    strPrint(buff, BUFF_SIZE, "USIZE_MAX = %zu", USIZE_MAX);
    Str dummy = strFromCstr(buff);
    strPrint(buff, BUFF_SIZE, "%.*s", (I32)dummy.len, dummy.ptr);
    printf("%.*s\n", (I32)dummy.len, dummy.ptr);

    memFree(alloc, buff, BUFF_SIZE);

    printf("-------------------------\n");
    printf("String builder\n");
    printf("-------------------------\n");

    StrBuilder sb = {0};

    strBuilderInit(&sb, alloc);
    strBuilderPrint(&sb, "%s", "This is");
    strBuilderAppendStr(&sb, strFromCstr(" a"));
    strBuilderAppend(&sb, " %s %s", "string", "buffer");
    printf("%s\n", strBuilderCstr(&sb));

    strBuilderShutdown(&sb);

    memFree(g_heap, fl_buffer, ALLOC_SIZE);

    //======================================================//

    Usize big_block_size = CF_GB(1);
    U8 *big_block = memAlloc(g_heap, big_block_size);
    FileIoToken token = {0};

    fileBeginWrite("C:/Temp/BigFile.bin", big_block, big_block_size, &token);

    while (!token.completed)
    {
        Sleep(1);
    }

    memFree(g_heap, big_block, big_block_size);
    printf("%s\n", token.success ? "SUCCESS" : "FAILURE");

    //======================================================//

    // Str filename = strLiteral("C:/Temp/Dummy.txt");
    // Clock clock;
    // IoFile file;
    // {
    //     file = fileOpen(filename, IoOpenMode_Write);
    //     for (Usize i = 0; i < 1000; ++i)
    //     {
    //         fileWriteStr(&file, strLiteral("This is a dummy file!\n"));
    //     }
    //     fileClose(&file);
    // }

    // Duration traw, tbuf;
    // Usize bcount = 0;
    // {
    //     clockStart(&clock);
    //     file = fileOpen(filename, IoOpenMode_Read);
    //     U8 buffer[1024] = {0};
    //     IoReader reader = {0};
    //     ioReaderInitFile(&reader, &file, buffer, CF_ARRAY_SIZE(buffer));
    //     U8 byte = {0};
    //     while (!ioReadByte(&reader, &byte)) bcount++;
    //     fileClose(&file);
    //     tbuf = clockElapsed(&clock);
    // }
    // {
    //     clockStart(&clock);
    //     file = fileOpen(filename, IoOpenMode_Read);
    //     U8 byte = {0};
    //     while (file.read(&file, &byte, 1)) bcount++;
    //     fileClose(&file);
    //     traw = clockElapsed(&clock);
    // }

    // printf("Read  %llu bytes\n", bcount);
    // printf("Raw read: %f\n", timeGetSeconds(traw));
    // printf("Buffered read: %f\n", timeGetSeconds(tbuf));

    return 0;
}
