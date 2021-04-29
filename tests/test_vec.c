#include "foundation/vec.h"

#include <stdio.h>

void
print_vec(Vec2 v)
{
    printf("{%f;%f}\n", (f64)v.x, (f64)v.y);
}

int
main(void)
{
    Vec2 a = {{1, 1}};
    Vec2 b = {{-3, -5}};
    Vec2 c = vecAdd(a, b);
    Vec2 d = vecDiv(c, 2.0);

    print_vec(a);
    print_vec(b);
    print_vec(c);
    print_vec(d);
}
