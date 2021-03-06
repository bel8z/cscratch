#include "paths.h"

#include "core.h"
#include "error.h"
#include "memory.h"
#include "strings.h"

#define DELIMITER_STR "\\/"

CF_GLOBAL const Str g_delimiters = {
    .buf = DELIMITER_STR,
    .len = CF_ARRAY_SIZE(DELIMITER_STR) - 1,
};

Str
pathSplitName(Str path)
{
    return pathSplitNameExt(path, NULL);
}

Str
pathSplitExt(Str path)
{
    Str ext = {0};
    pathSplitNameExt(path, &ext);
    return ext;
}

Str
pathSplitNameExt(Str path, Str *ext)
{
    Usize delim_pos = strFindLast(path, g_delimiters);

    if (delim_pos != USIZE_MAX)
    {
        path.buf += delim_pos;
        path.len -= delim_pos;

        while (path.len && strContains(g_delimiters, path.buf[0]))
        {
            path.buf++;
            path.len--;
        }
    }

    if (ext && path.len)
    {
        // TODO (Matteo): Improve
        Usize ext_pos = strFindLast(path, strLiteral("."));

        if (ext_pos == USIZE_MAX)
        {
            ext->buf = NULL;
            ext->len = 0;
        }
        else
        {
            ext->buf = path.buf + ext_pos;
            ext->len = path.len - ext_pos;
        }
    }

    return path;
}

Usize
pathJoin(Str root, Str leaf, Char8 *buffer, Usize buffer_size)
{
    Usize size = root.len + leaf.len + 1;

    if (buffer)
    {
        if (size > buffer_size) return USIZE_MAX;

        memCopy(root.buf, buffer, root.len);
        buffer += root.len;

        *buffer = '/';
        buffer++;

        memCopy(leaf.buf, buffer, leaf.len);
    }

    return size;
}

Usize
pathJoinBuf(Str root, Str leaf, StrBuffer *buffer)
{
    Usize req_len = pathJoin(root, leaf, buffer->data, CF_ARRAY_SIZE(buffer->data));
    if (req_len != USIZE_MAX) buffer->str.len = req_len;
    return req_len;
}

Usize
pathChangeExt(Str path, Str new_ext, Char8 *out)
{
    Str ext = pathSplitExt(path);

    CF_ASSERT_NOT_NULL(ext.buf);

    Isize offset = path.len - ext.len;

    if (out)
    {
        memCopy(path.buf, out, offset);
        memCopy(new_ext.buf, out + offset, new_ext.len);
    }

    return offset + new_ext.len;
}

void
pathSplitStart(PathSplitIter *iter, Str path)
{
    CF_ASSERT_NOT_NULL(iter);

    iter->path = path;
    iter->curr.buf = NULL;
    iter->curr.len = 0;
}

bool
pathSplitNext(PathSplitIter *iter)
{
    // TODO (Matteo): Handle network paths (which start with a double delimiter)
    CF_ASSERT_NOT_NULL(iter);

    if (!strValid(iter->path)) return false;

    if (!strValid(iter->curr))
    {
        iter->curr = iter->path;
    }
    else
    {
        Usize offset = iter->curr.buf + iter->curr.len - iter->path.buf;
        iter->curr.buf = iter->path.buf + offset;
        iter->curr.len = iter->path.len - offset;
    }

    while (iter->curr.len && strContains(g_delimiters, iter->curr.buf[0]))
    {
        iter->curr.buf++;
        iter->curr.len--;
    }

    // Find next separator to terminate the string
    Usize delim_pos = strFindFirst(iter->curr, g_delimiters);
    if (delim_pos != USIZE_MAX) iter->curr.len = delim_pos;

    return !!(iter->curr.len);
}
