#pragma once

/// Path manipulation utilities
/// This is not an API header, include it in implementation files only

#include "core.h"
#include "strings.h" // For strValid

Str pathSplitName(Str path);
Str pathSplitExt(Str path);
Str pathSplitNameExt(Str path, Str *ext);

Usize pathChangeExt(Str path, Str new_ext, Char8 *out);

typedef struct PathSplitIter
{
    Str curr;
    Str path;
} PathSplitIter;

void pathSplitStart(PathSplitIter *iter, Str path);
bool pathSplitNext(PathSplitIter *iter);
