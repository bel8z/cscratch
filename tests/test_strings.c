#include "foundation/string_list.h"

#include <stdio.h>

static u8 buffer[4096];

int
main(void)
{
    StringList sl = {0};
    slInitBuffer(&sl, buffer, 4096);

    usize i = 0;

    while (slPush(&sl, __FILE__))
    {
        ++i;
    }

    printf("Pushed %zu strings\n", i);

    cfList *cursor = sl.sentinel.next;

    while (cursor != &sl.sentinel)
    {
        StringEntry *entry = cfListItem(cursor, StringEntry, node);
        printf("Data: %s\n", entry->data);
        printf("Size: %zu\n", entry->size);
    }

    return 0;
}
