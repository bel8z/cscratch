
// Windows thread pool IO

#include "platform.h"

#include "foundation/core.h"
#include "foundation/memory.h"
#include "foundation/win32.inl"

// TODO (Matteo): Get rid of it and use platform API only
#include <stdio.h>

static MemAllocator g_heap;

#define IO_CALLBACK(name) void name(void *context, ULONG result, ULONG_PTR bytes)

typedef IO_CALLBACK((*IoCallback));

typedef struct IoContext
{
    IoCallback callback;
    void *user_data;
    OVERLAPPED ovp;
} IoContext;

typedef struct FileIoToken
{
    bool completed;
    bool success;
} FileIoToken;

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

static TP_IO *
ioBegin(IoContext *context, HANDLE file)
{
    TP_IO *io = CreateThreadpoolIo(file, ioCallback, context, NULL);
    StartThreadpoolIo(io);
    return io;
}

static IO_CALLBACK(fileIoCallback)
{
    CF_UNUSED(bytes);
    FileIoToken *token = context;
    token->success = !result;
    token->completed = true;
}

static void
fileBeginWrite(Cstr filename, U8 const *buffer, Size size, FileIoToken *token)
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

I32
consoleMain(Platform *platform, CommandLine *cmd_line)
{
    CF_UNUSED(cmd_line);

    g_heap = platform->heap;
    Size big_block_size = CF_GB(1);
    U8 *big_block = memAlloc(g_heap, big_block_size);
    FileIoToken token = {0};

    fileBeginWrite("C:/Temp/BigFile.bin", big_block, big_block_size, &token);

    while (!token.completed)
    {
        Sleep(1);
    }

    memFree(g_heap, big_block, big_block_size);
    printf("%s\n", token.success ? "SUCCESS" : "FAILURE");

    return 0;
}
