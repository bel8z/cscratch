#include "foundation/core.h"

#include <stdio.h>

typedef struct Platform Platform;

bool testBenaphore(Platform *platform);
bool testAutoResetEvent(Platform *platform);
bool testMpmcQueue(Platform *platform);

I32
platformMain(Platform *platform, Cstr argv[], I32 argc)
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
        case 0: result = testBenaphore(platform); break;
        case 1: result = testAutoResetEvent(platform); break;
        case 2: result = testMpmcQueue(platform); break;
        default: break;
    }

    if (!result) return -3;

    return 0;
}
