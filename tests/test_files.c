#include "foundation/core.h"
#include "foundation/paths.h"

#include <windows.h>

#include <stdio.h>

void
pathPrint(Str p)
{
    if (strValid(p)) printf("%.*s\n", (I32)p.len, p.buf);
}

int
main(void)
{
    printf("Splitting paths:\n");

    Str p = {0};

    p = pathSplitName(strFromCstr("c:\\Temp/IOT\\2021_02_25/"));
    pathPrint(p);

    p = pathSplitName(
        strFromCstr("//srvfile/AreaComune/Utenti/Matteo.Belotti/Lavori/Generatore/!Src/"
                    "EyeGoal-COREStation.zip"));
    pathPrint(p);

    p = pathSplitName(strFromCstr("Gianni Morandi"));
    pathPrint(p);

    Str dirname = strFromCstr("c:\\Temp/IOT\\2021_02_25/");
    Str filename = strFromCstr("//srvfile/AreaComune/Utenti/Matteo.Belotti/Lavori/Generatore/!Src/"
                               "EyeGoal-COREStation.zip");

    PathSplitIter pi = {0};
    pathSplitStart(&pi, dirname);

    while (pathSplitNext(&pi))
    {
        pathPrint(pi.curr);
    }

    pathSplitStart(&pi, filename);

    while (pathSplitNext(&pi))
    {
        pathPrint(pi.curr);
    }

    // printf("Browsing dir:\n");

    // Platform plat = cfPlatformCreate();
    // DirIter *iter = plat.fs.dirIterStart(dirname, &plat.heap);

    // if (iter)
    // {
    //     char const *f = NULL;
    //     while ((f = plat.fs.dirIterNext(iter)))
    //     {
    //         printf("%s\n", f);
    //     }

    //     plat.fs.dirIterClose(iter);
    // }

    // U32 sz = 0;
    // char *f = plat.fs.open_file_dlg(NULL, NULL, &plat.heap, &sz);
    // printf("Opened file: %s \n", f);
    // cfFree(&plat.heap, f, sz);

    // cfAllocatorStats s = cfAllocStats(&plat.heap);
    // CF_ASSERT(s.count == 0, "LEAK!");
    // CF_ASSERT(s.size == 0, "LEAK!");

    // cfPlatformShutdown(&plat);

    return 0;
}
