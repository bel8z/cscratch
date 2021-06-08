#include "foundation/common.h"
#include "foundation/fs.h"
#include "foundation/path.h"
#include "foundation/platform.h"

#include <windows.h>

#include <stdio.h>

int
main(void)
{
    printf("Splitting paths:\n");

    char const *p = NULL;

    p = pathSplitName("c:\\Temp/IOT\\2021_02_25/");
    if (p) printf("%s\n", p);

    p = pathSplitName("//srvfile/AreaComune/Utenti/Matteo.Belotti/Lavori/Generatore/!Src/"
                      "EyeGoal-COREStation.zip");
    if (p) printf("%s\n", p);

    p = pathSplitName("Gianni Morandi");
    if (p) printf("%s\n", p);

    char const *dirname = "c:\\Temp/IOT\\2021_02_25/";
    char const *filename = "//srvfile/AreaComune/Utenti/Matteo.Belotti/Lavori/Generatore/!Src/"
                           "EyeGoal-COREStation.zip";

    PathSplitIter pi = {0};
    pathSplitStart(&pi, dirname);

    while (pathSplitNext(&pi))
    {
        printf("%.*s\n", (int)pi.len, pi.cur);
    }

    pathSplitStart(&pi, filename);

    while (pathSplitNext(&pi))
    {
        printf("%.*s\n", (int)pi.len, pi.cur);
    }

    printf("Browsing dir:\n");

    cfPlatform plat = cfPlatformCreate();
    DirIter *iter = plat.fs.dir_iter_start(dirname, &plat.heap);

    if (iter)
    {
        char const *f = NULL;
        while ((f = plat.fs.dir_iter_next(iter)))
        {
            printf("%s\n", f);
        }

        plat.fs.dir_iter_close(iter);
    }

    U32 sz = 0;
    char *f = plat.fs.open_file_dlg(NULL, NULL, &plat.heap, &sz);
    printf("Opened file: %s \n", f);
    cfFree(&plat.heap, f, sz);

    cfAllocatorStats s = cfAllocStats(&plat.heap);
    CF_ASSERT(s.count == 0, "LEAK!");
    CF_ASSERT(s.size == 0, "LEAK!");

    cfPlatformShutdown(&plat);

    return 0;
}
