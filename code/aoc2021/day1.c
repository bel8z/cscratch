#include "platform.h"

#include "foundation/core.h"
#include "foundation/error.h"
#include "foundation/fs.h"
#include "foundation/io.h"
#include "foundation/memory.h"
#include "foundation/paths.h"
#include "foundation/strings.h"

#include <stdio.h>

File
appOpenInput(Platform *platform, Str filename)
{
    Char8 buffer[1024];

    Str path = {
        .buf = buffer,
        .len = pathJoin(platform->paths->base, filename, buffer, CF_ARRAY_SIZE(buffer)),
    };

    CF_ASSERT(path.len < CF_ARRAY_SIZE(buffer), "");

    return fileOpen(path, FileOpenMode_Read);
}

I32
platformMain(Platform *platform, CommandLine *cmd_line)
{
    CF_UNUSED(platform);
    CF_UNUSED(cmd_line);

    File in_file = appOpenInput(platform, strLiteral("input/day1"));
    U8 in_buffer[4096] = {0};
    IoReader in = {0};
    ioReaderInitFile(&in, &in_file, in_buffer, CF_ARRAY_SIZE(in_buffer));

    U8 line[1024] = {0};
    Usize line_size = 0;

    while (!ioReadLine(&in, 1024, line, &line_size))
    {
        fprintf(stderr, "\"");
        fwrite(line, sizeof(*line), line_size, stderr);
        fprintf(stderr, "\"\n");
    }

    return 0;
}
