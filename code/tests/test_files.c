#include "platform.h"

#include "foundation/core.h"
#include "foundation/io.h"
#include "foundation/paths.h"
#include "foundation/strings.h"

#include <stdio.h>

typedef struct Platform Platform;
typedef struct CommandLine CommandLine;

void
pathPrint(Str p)
{
    if (strValid(p)) printf("%.*s\n", (I32)p.len, p.ptr);
}

I32
consoleMain(Platform *platform, CommandLine *cmd_line)
{
    CF_UNUSED(cmd_line);

    IoFileApi *file = platform->file;

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
    IoDirectory iter = {0};

    if (file->dirOpen(&iter, dirname))
    {
        Str f = {0};
        while (iter.next(&iter, &f, NULL))
        {
            pathPrint(f);
        }

        iter.close(&iter);
    }

    //======================================================//

    // Str filename = strLiteral("C:/Temp/Dummy.txt");
    // Clock clock;
    // IoFile file;
    // {
    //     file = fileOpen(filename, IoOpenMode_Write);
    //     for (Size i = 0; i < 1000; ++i)
    //     {
    //         fileWriteStr(&file, strLiteral("This is a dummy file!\n"));
    //     }
    //     fileClose(&file);
    // }

    // Duration traw, tbuf;
    // Size bcount = 0;
    // {
    //     clockStart(&clock);
    //     file = fileOpen(filename, IoOpenMode_Read);
    //     U8 buffer[1024] = {0};
    //     IoReader reader = {0};
    //     ioReaderInitFile(&reader, &file, buffer, CF_ARRAY_SIZE(buffer));
    //     U8 byte = {0};
    //     while (!ioReadByte(&reader, &byte)) bcount++;
    //     fileClose(&file);
    //     tbuf = clockElapsed(&clock);
    // }
    // {
    //     clockStart(&clock);
    //     file = fileOpen(filename, IoOpenMode_Read);
    //     U8 byte = {0};
    //     while (file.read(&file, &byte, 1)) bcount++;
    //     fileClose(&file);
    //     traw = clockElapsed(&clock);
    // }

    // printf("Read  %llu bytes\n", bcount);
    // printf("Raw read: %f\n", timeGetSeconds(traw));
    // printf("Buffered read: %f\n", timeGetSeconds(tbuf));

    return 0;
}
