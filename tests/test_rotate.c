#include <stdio.h>
#include <stdlib.h>

#include "foundation/util.h"

void
arrayPrint(i32 const *array, usize count)
{
    printf("{");
    for (usize i = 0; i < count; ++i)
    {
        printf("%d,", array[i]);
    }
    printf("}\n");
}

int
main(void)
{
    for (usize pos = 0; pos < 6; ++pos)
    {
        i32 temp[] = {0, 1, 2, 3, 4, 5};
        cfRotateLeft(temp, 6, pos);
        arrayPrint(temp, 6);
    }

    for (usize pos = 0; pos < 6; ++pos)
    {
        i32 temp[] = {0, 1, 2, 3, 4, 5};
        cfRotateRight(temp, 6, pos);
        arrayPrint(temp, 6);
    }

    return 0;
}
