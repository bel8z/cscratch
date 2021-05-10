#include "foundation/common.h"

#include "foundation/util.h"

#include <stdlib.h>

#define DEFAULT_ALIGNMENT (sizeof(void *) * 2)

typedef struct Arena
{
    u8 *buf;
    usize cap;
    usize curr;
    usize prev;
} Arena;

typedef struct ArenaTempState
{
    Arena *arena;
    usize curr;
    usize prev;
} ArenaTempState;

void
arenaInit(Arena *arena, u8 *buffer, usize buffer_size)
{
    arena->buf = buffer;
    arena->cap = buffer_size;
    arena->curr = 0;
    arena->prev = 0;
}

void *
arenaAllocAlign(Arena *arena, usize size, usize align)
{
    CF_ASSERT((align & (align - 1)) == 0, "alignment is not a power of 2");

    uptr curr_ptr = (uptr)(arena->buf + arena->curr);
    uptr modulo = curr_ptr & (align - 1);

    curr_ptr += align - modulo;

    uptr offset = curr_ptr + align - modulo - (uptr)arena->buf;

    if (offset + size > arena->cap) return NULL;

    void *result = arena->buf + offset;

    arena->prev = offset;
    arena->curr = offset + size;

    cfMemClear(result, size);

    return result;
}

void *
arenaAlloc(Arena *arena, usize size)
{
    return arenaAllocAlign(arena, size, DEFAULT_ALIGNMENT);
}

#define arenaAllocStruct(arena, Type) arenaAllocAlign(arena, sizeof(Type), alignof(Type))

#define arenaAllocArray(arena, Type, count) \
    arenaAllocAlign(arena, count * sizeof(Type), alignof(Type))

ArenaTempState
arenaSaveState(Arena *arena)
{
    return (ArenaTempState){.arena = arena, .prev = arena->prev, .curr = arena->curr};
}

void
arenaRestoreState(Arena *arena, ArenaTempState state)
{
    CF_ASSERT(arena == state.arena, "Restoring invalid state");

    arena->prev = state.prev;
    arena->curr = state.curr;
}

int
main(void)
{
    Arena arena = {0};

    arenaInit(&arena, malloc(1024 * 1024), 1024 * 1024);

    i32 *ints = arenaAllocArray(&arena, i32, 1024);

    CF_ASSERT_NOT_NULL(ints);

    free(arena.buf);

    return 0;
}
