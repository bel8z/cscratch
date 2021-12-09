#pragma once

/// Path manipulation utilities
/// This is not an API header, include it in implementation files only

#include "core.h"

CF_API Str pathSplitName(Str path);
CF_API Str pathSplitExt(Str path);
CF_API Str pathSplitNameExt(Str path, Str *ext);

CF_API Usize pathJoin(Str root, Str leaf, Char8 *buffer, Usize buffer_size);

CF_API Usize pathChangeExt(Str path, Str new_ext, Char8 *out);

typedef struct PathSplitIter
{
    Str curr;
    Str path;
} PathSplitIter;

CF_API void pathSplitStart(PathSplitIter *iter, Str path);
CF_API bool pathSplitNext(PathSplitIter *iter);
