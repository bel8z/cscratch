#include "foundation/core.h"
#include "foundation/util.h"

#define SORT_CMP_FN(name) bool name(void *a, void *b, void *state)
#define SORT_FN(name) void name(void *items, Usize item_size, Usize item_count, SortCtx ctx)

typedef struct SortCtx
{
    SORT_CMP_FN((*cmp));
    void *state;
} SortCtx;

SORT_FN(mergeSort);
SORT_FN(quickSort);
SORT_FN(bubbleSort);

static inline bool
greater(void *a, void *b, SortCtx ctx)
{
    return ctx.cmp(a, b, ctx.state);
}

SORT_FN(bubbleSort)
{
    Usize n = item_count;
    while (n > 0)
    {
        Usize next_n = 0;
        U8 *prev = items;
        U8 *curr;

        for (Usize i = 1; i < n; ++i, prev = curr)
        {
            curr = prev + item_size;

            if (greater(prev, curr, ctx))
            {
                cfSwapBytes(prev, curr, item_size);
                next_n = i; // Update upper bound for next linear scan
            }
        }

        n = next_n;
    }
}

#define PRIMITIVE_CMP(Type)                 \
    SORT_CMP_FN(cmp##Type)                  \
    {                                       \
        CF_UNUSED(state);                   \
        return *(Type *)(a) > *(Type *)(b); \
    }

PRIMITIVE_CMP(U8)
PRIMITIVE_CMP(U16)
PRIMITIVE_CMP(U32)
PRIMITIVE_CMP(U64)
PRIMITIVE_CMP(Usize)
PRIMITIVE_CMP(Uptr)

PRIMITIVE_CMP(I8)
PRIMITIVE_CMP(I16)
PRIMITIVE_CMP(I32)
PRIMITIVE_CMP(I64)
PRIMITIVE_CMP(Isize)
PRIMITIVE_CMP(Iptr)

PRIMITIVE_CMP(F32)
PRIMITIVE_CMP(F64)

#undef PRIMITIVE_CMP

#include "platform.h"
#include <stdio.h>

I32
consoleMain(Platform *platform, CommandLine *cmd_line)
{
    CF_UNUSED(platform);
    CF_UNUSED(cmd_line);

    SortCtx ctx = {.cmp = cmpI32};
    I32 ints[] = {5, -2, 4, 0, 1, 2, 8, 3, 6, -1, 7};

    bubbleSort(ints, sizeof(I32), CF_ARRAY_SIZE(ints), ctx);

    for (I32 i = -2; i < 9; ++i)
    {
        printf("%d, ", ints[i + 2]);
        CF_ASSERT(i == ints[i + 2], "Sort failure");
    }

    printf("\n");

    return 0;
}
