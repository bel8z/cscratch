#include "platform.h"

#include "foundation/core.h"
#include "foundation/fs.h"
#include "foundation/paths.h"
#include "foundation/strings.h"

#include <stdio.h>

void
pathPrint(Str p)
{
    if (strValid(p)) printf("%.*s\n", (I32)p.len, p.buf);
}

I32
platformMain(Platform *platform, Cstr argv[], I32 argc)
{
    CF_UNUSED(platform);
    CF_UNUSED(argv);
    CF_UNUSED(argc);

    printf("Splitting paths:\n");

    Str p = {0};

    p = pathSplitName(strLiteral("c:\\Temp/IOT\\2021_02_25/"));
    pathPrint(p);

    p = pathSplitName(
        strLiteral("//srvfile/AreaComune/Utenti/Matteo.Belotti/Lavori/Generatore/!Src/"
                   "EyeGoal-COREStation.zip"));
    pathPrint(p);

    p = pathSplitName(strLiteral("Gianni Morandi"));
    pathPrint(p);

    Str dirname = strLiteral("c:\\Temp/IOT\\2021_02_25/");
    Str filename = strLiteral("//srvfile/AreaComune/Utenti/Matteo.Belotti/Lavori/Generatore/!Src/"
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

    printf("Browsing dir:\n");

    // Platform plat = cfPlatformCreate();
    FsIterator iter = {0};

    if (fsIteratorStart(&iter, dirname))
    {
        Str f = {0};
        while (fsIteratorNext(&iter, &f, NULL))
        {
            pathPrint(f);
        }

        fsIteratorEnd(&iter);
    }

    // U32 sz = 0;
    // char *f = plat.fs.fileOpenDialog(NULL, NULL, &plat.heap, &sz);
    // printf("Opened file: %s \n", f);
    // cfFree(&plat.heap, f, sz);

    // cfAllocatorStats s = cfAllocStats(&plat.heap);
    // CF_ASSERT(s.count == 0, "LEAK!");
    // CF_ASSERT(s.size == 0, "LEAK!");

    // cfPlatformShutdown(&plat);

    return 0;
}
