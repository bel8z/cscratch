#include "platform.h"

#include "foundation/core.h"
#include "foundation/error.h"
#include "foundation/fs.h"
#include "foundation/memory.h"
#include "foundation/paths.h"
#include "foundation/strings.h"

#include <stdio.h>

typedef struct InputFile
{
    File file;
    Char8 path_buf[1024];
    U8 read_buf[4096];
    Usize read_size;
} InputFile;

void
inFileOpen(InputFile *in, Platform *platform, Str filename)
{
    Str path = {
        .buf = in->path_buf,
        .len = pathJoin(platform->paths->base, filename, in->path_buf, CF_ARRAY_SIZE(in->path_buf)),
    };
    CF_ASSERT(path.len < CF_ARRAY_SIZE(in->path_buf), "");

    in->file = fileOpen(path, FileOpenMode_Read);
}

bool
inFileFillBuffer(InputFile *in)
{
    in->read_size = in->file.read(&in->file, in->read_buf, CF_ARRAY_SIZE(in->read_buf));
    return (in->read_size > 0);
}

I32
platformMain(Platform *platform, CommandLine *cmd_line)
{
    CF_UNUSED(platform);
    CF_UNUSED(cmd_line);

    InputFile in = {0};
    inFileOpen(&in, platform, strLiteral("input/day1"));

    while (inFileFillBuffer(&in))
    {
        fwrite(in.read_buf, sizeof(*in.read_buf), in.read_size, stderr);
    }

    return 0;
}
