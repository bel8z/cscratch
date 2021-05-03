#ifndef FOUNDATION_FS_H

#include "common.h"

//------------------------------------------------------------------------------

// Iterates over directory entries
// The platform specific structs are abstracted and packed in the opaque buffer
typedef struct DirIter
{
    u8 opaque[512];
} DirIter;

// Start iteration of the given directory
bool dirIterStart(DirIter *iter, char const *dir);
// Advance the iterator and return the filename of the current entry, or NULL
// if the iteration is complete; NOTE that the current pointer is valid
// until the next call to this function (or the iterator is destroyed)
char const *dirIterNext(DirIter *iter);
// Shutdown the iteration (this is a requirement for some underlying platform specific
// implementations)
void dirIterClose(DirIter *iter);

//------------------------------------------------------------------------------

#define FOUNDATION_FS_H
#endif
