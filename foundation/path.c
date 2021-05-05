#include "path.h"

#include "common.h"

#include <string.h>

static char const *g_delimiters = "\\/";

char const *
pathSplitName(char const *path)
{
    return pathSplitNameExt(path, NULL);
}

char const *
pathSplitExt(char const *path)
{
    char const *ext;
    return pathSplitNameExt(path, &ext) ? ext : NULL;
}

char const *
pathSplitNameExt(char const *path, char const **ext)
{

    while (path)
    {
        char const *temp = strpbrk(path, g_delimiters);
        if (!temp) break;
        path = temp + strspn(temp, g_delimiters);
    }

    // TODO (Matteo): Implement
    if (ext) *ext = NULL;

    return path;
}

void
pathSplitStart(PathSplitIter *iter, char const *path)
{
    CF_ASSERT_NOT_NULL(iter);

    iter->path = path;
    iter->cur = NULL;
    iter->len = 0;
}

bool
pathSplitNext(PathSplitIter *iter)
{
    CF_ASSERT_NOT_NULL(iter);

    if (!iter->path) return false;

    iter->cur = iter->cur ? iter->cur + iter->len : iter->path;

    char const *next = strpbrk(iter->cur, g_delimiters);

    if (next)
    {
        // TODO (Matteo): Handle network paths (which start with a double separator)
        iter->len = next + strspn(next, g_delimiters) - iter->cur;
    }
    else
    {
        iter->len = 0;
        while (iter->cur[iter->len])
        {
            iter->len++;
        }
    }

    return !!(iter->len);
}