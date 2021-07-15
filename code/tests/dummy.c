#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "foundation/list.h"
#include "foundation/memory.h"
#include "foundation/strings.h"

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

#define ALLOC_SIZE CF_MB(1)
#define BUFF_SIZE 1024

int
main()
{
    FreeListAlloc fl = {0};
    freeListAllocInit(&fl, ALLOC_SIZE);
    CfAllocator alloc = freeListAllocator(&fl);

    Char8 *buff = cfAlloc(alloc, BUFF_SIZE);

    strPrintf(buff, BUFF_SIZE, "USIZE_MAX = %zu", USIZE_MAX);
    Str dummy = strFromCstr(buff);
    strPrintf(buff, BUFF_SIZE, "%.*s", (I32)dummy.len, dummy.buf);
    printf("%.*s\n", (I32)dummy.len, dummy.buf);

    cfFree(alloc, buff, BUFF_SIZE);

    return 0;
}
