#ifndef FOUNDATION_PATH_H

#include "common.h"

char const *pathSplitName(char const *path);
char const *pathSplitExt(char const *path);
char const *pathSplitNameExt(char const *path, char const **ext);

typedef struct PathSplitIter
{
    char const *cur;
    usize len;
    // **Internal**
    char const *path;
} PathSplitIter;

void pathSplitStart(PathSplitIter *iter, char const *path);
bool pathSplitNext(PathSplitIter *iter);

#define FOUNDATION_PATH_H
#endif