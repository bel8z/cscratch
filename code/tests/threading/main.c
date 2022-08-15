#include "platform.h"

#include "foundation/core.h"

#include <stdio.h>

bool testBenaphore(Platform *platform);
bool testAutoResetEvent(Platform *platform);
bool testMpmcQueue(Platform *platform);
bool testBasic(Platform *platform);

I32
consoleMain(Platform *platform, CommandLine *cmd_line)
{
    bool result = false;

    if (cmd_line->len != 2)
    {
        result = testBasic(platform);
    }
    else
    {
        I32 code = -1;

        if (sscanf_s(cmd_line->arg[1], "%d", &code) != 1)
        {
            return -2;
        }

        switch (code)
        {
            case 0: result = testBenaphore(platform); break;
            case 1: result = testAutoResetEvent(platform); break;
            case 2: result = testMpmcQueue(platform); break;
            default: break;
        }
    }

    if (!result) return -3;

    return 0;
}
