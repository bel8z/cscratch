#include "core.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

bool
from_string(const char *str, Usize *val)
{
    Usize len = strlen(str);

    if (len > CHAR_BIT * sizeof(*val)) return false;

    for (Usize i = 0; i < len; ++i)
    {
        Usize bit = str[len - i - 1] - 0x30;

        switch (bit)
        {
            case 0:
            case 1: *val |= bit << i; break;
            default: return false;
        }
    }

    return true;
}

void
test_bin_string(char const *str)
{
    Usize val = 0;

    printf("%s => ", str);

    if (from_string(str, &val))
    {
        printf("%zu", val);
    }
    else
    {
        printf("???");
    }

    printf("\n");
}

int
main()
{

    test_bin_string("011000111");
    test_bin_string("011050111");
    test_bin_string("0110001111111111111111111111111111111111111111111111");
    test_bin_string("1111111111111111111011000111111111111111111111111111111111"
                    "1111111111111");

    return 0;
}
