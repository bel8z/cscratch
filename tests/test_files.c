#include "foundation/common.h"

#include <windows.h>

#include <stdio.h>
#include <string.h>

int
main(void)
{
    printf("Splitting paths:\n");

    char const *filename = "c:\\Temp/IOT\\2021_02_25/*";
    // char const *filename = "//srvfile/AreaComune/Utenti/Matteo.Belotti/Lavori/Generatore/!Src/"
    //                        "EyeGoal-COREStation.zip";
    char const *delims = "\\/";

    for (char const *cursor = filename; *cursor;)
    {
        // cursor += strspn(cursor, delims);
        size_t len = strcspn(cursor, delims);
        len += strspn(cursor + len, delims);
        printf("%.*s\n", (int)len, cursor);
        cursor += len;
    }

    printf("Browsing dir:\n");

    WIN32_FIND_DATAA data;
    HANDLE finder = FindFirstFileA(filename, &data);

    if (finder != INVALID_HANDLE_VALUE)
    {
        printf("%s\n", data.cFileName);

        while (FindNextFileA(finder, &data))
        {
            printf("%s\n", data.cFileName);
        }

        FindClose(finder);
    }

    return 0;
}
