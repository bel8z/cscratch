#include "paths.h"

#include "core.h"
#include "error.h"
#include "memory.h"
#include "strings.h"

#define G_DELIMITERS "\\/"

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
    Str delim = strLiteral(G_DELIMITERS);
    Size delim_pos = strFindLast(path, delim);

    if (delim_pos != SIZE_MAX)
    {
        path.ptr += delim_pos;
        path.len -= delim_pos;

        while (path.len && strContains(delim, path.ptr[0]))
        {
            path.ptr++;
            path.len--;
        }
    }

    if (ext && path.len)
    {
        // TODO (Matteo): Improve
        Size ext_pos = strFindLast(path, strLiteral("."));

        if (ext_pos == SIZE_MAX)
        {
            ext->ptr = NULL;
            ext->len = 0;
        }
        else
        {
            ext->ptr = path.ptr + ext_pos;
            ext->len = path.len - ext_pos;
        }
    }

    return path;
}

Size
pathJoin(Str root, Str leaf, Char8 *buffer, Size buffer_size)
{
    Size size = root.len + leaf.len + 1;

    if (buffer)
    {
        if (size > buffer_size) return SIZE_MAX;

        memCopy(root.ptr, buffer, root.len);
        buffer += root.len;

        *buffer = '/';
        buffer++;

        memCopy(leaf.ptr, buffer, leaf.len);
    }

    return size;
}

Size
pathJoinBuf(Str root, Str leaf, StrBuffer *buffer)
{
    Size req_len = pathJoin(root, leaf, buffer->data, CF_ARRAY_SIZE(buffer->data));
    if (req_len != SIZE_MAX) buffer->str.len = req_len;
    return req_len;
}

Size
pathChangeExt(Str path, Str new_ext, Char8 *out)
{
    Str ext = pathSplitExt(path);

    CF_ASSERT_NOT_NULL(ext.ptr);

    Size offset = path.len - ext.len;

    if (out)
    {
        memCopy(path.ptr, out, offset);
        memCopy(new_ext.ptr, out + offset, new_ext.len);
    }

    return offset + new_ext.len;
}

void
pathSplitStart(PathSplitIter *iter, Str path)
{
    CF_ASSERT_NOT_NULL(iter);

    iter->path = path;
    iter->curr.ptr = NULL;
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
        Size offset = (Size)(iter->curr.ptr + iter->curr.len - iter->path.ptr);
        iter->curr.ptr = iter->path.ptr + offset;
        iter->curr.len = iter->path.len - offset;
    }

    Str delim = strLiteral(G_DELIMITERS);

    while (iter->curr.len && strContains(delim, iter->curr.ptr[0]))
    {
        iter->curr.ptr++;
        iter->curr.len--;
    }

    // Find next separator to terminate the string
    Size delim_pos = strFindFirst(iter->curr, delim);
    if (delim_pos != SIZE_MAX) iter->curr.len = delim_pos;

    return !!(iter->curr.len);
}
