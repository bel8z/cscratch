#ifndef IMAGE_H

#include "foundation/allocator.h"
#include "foundation/common.h"

// TODO (Matteo):
// * Keep image bits around (for querying/manipulation)?
// * Implement image view navigation (scale/offset)
// * Implement a file queue for browsing
// * Implement image rotation

typedef struct Image
{
    u32 texture;
    i32 width;
    i32 height;
} Image;

// Simple helper function to load an image into a OpenGL texture with common settings
bool imageLoadFromFile(Image *image, const char *filename, cfAllocator *alloc);

#define IMAGE_H
#endif
