#include "foundation/vec.h"

#include <stdio.h>

typedef struct Platform Platform;
typedef struct CommandLine CommandLine;

void
print_vec(Vec2 v)
{
    printf("{%f;%f}\n", (F64)v.x, (F64)v.y);
}

I32
platformMain(Platform *platform, CommandLine *cmd_line)
{
    CF_UNUSED(platform);
    CF_UNUSED(cmd_line);

    Vec2 a = {{1, 1}};
    Vec2 b = {{-3, -5}};
    Vec2 c = vecAdd(a, b);
    Vec2 d = vecDiv(c, 2.0);

    print_vec(a);
    print_vec(b);
    print_vec(c);
    print_vec(d);
}
