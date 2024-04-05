#pragma once

/// Path manipulation utilities
/// This is not an API header, include it in implementation files only

#include "core.h"

CF_INLINE_API Str pathSplitName(Str path);
CF_INLINE_API Str pathSplitExt(Str path);
CF_API Str pathSplitNameExt(Str path, Str *ext);

CF_API Size pathJoin(Str root, Str leaf, Char8 *buffer, Size buffer_size);
CF_API Size pathJoinBuf(Str root, Str leaf, StrBuffer *buffer);

CF_API Size pathChangeExt(Str path, Str new_ext, Char8 *out);

typedef struct PathSplitIter
{
    Str curr;
    Str path;
} PathSplitIter;

CF_INLINE_API void pathSplitStart(PathSplitIter *iter, Str path);
CF_API bool pathSplitNext(PathSplitIter *iter);
