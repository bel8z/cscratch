#include "foundation/core.h"

#include <stdio.h>

bool testBenaphore(void);
bool testAutoResetEvent(void);
bool testMpmcQueue(void);

I32
main(I32 argc, Cstr argv[])
{

    if (argc != 2)
    {
        return -1;
    }

    I32 code = -1;

    if (sscanf_s(argv[1], "%d", &code) != 1)
    {
        return -2;
    }

    bool result = false;

    switch (code)
    {
        case 0: result = testBenaphore(); break;
        case 1: result = testAutoResetEvent(); break;
        case 2: result = testMpmcQueue(); break;
        default: break;
    }

    if (!result) return -3;

    return 0;
}
