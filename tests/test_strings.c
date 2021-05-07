#include "foundation/common.h"
#include "foundation/string_list.h"

#include <stdio.h>

static u8 g_buffer[4096];

int
main(void)
{
    StringList buff = {0};

    slInitBuffer(&buff, g_buffer, sizeof(g_buffer));

    usize i = 0;

    if (slPush(&buff, "STR1")) ++i;
    if (slPush(&buff, "STR2")) ++i;
    if (slPush(&buff, "STR3")) ++i;

    printf("Pushed %zu strings\n", i);

    StringEntry *entry = NULL;

    while (slIterNext(&buff, &entry))
    {
        printf("Data: %s\n", entry->data);
        printf("Size: %zu\n", entry->size);
    }

    CF_ASSERT(slPop(&buff), "");
    CF_ASSERT(slPop(&buff), "");
    CF_ASSERT(slPop(&buff), "");
    CF_ASSERT(!slPop(&buff), "");

    entry = NULL;

    while (slIterNext(&buff, &entry))
    {
        printf("Data: %s\n", entry->data);
        printf("Size: %zu\n", entry->size);
    }

    return 0;
}
