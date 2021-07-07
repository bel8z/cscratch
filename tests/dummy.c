#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "foundation/strings.h"

int
main()
{
    char buff[1024];
    strPrintf(buff, 1024, "SIZE_MAX = %zu", SIZE_MAX);
    Str dummy = strFromCstr(buff);
    strPrintf(buff, 1024, "%.*s", (I32)dummy.len, dummy.buf);
    printf("%.*s\n", (I32)dummy.len, dummy.buf);
    return 0;
}
