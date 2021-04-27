#ifndef IMAGE_H

#include "foundation/allocator.h"
#include "foundation/common.h"

#include <GL/gl3w.h>

typedef struct Image
{

    GLuint texture;
    i32 width;
    i32 height;
} Image;

// Simple helper function to load an image into a OpenGL texture with common settings
bool imageLoadFromFile(Image *image, const char *filename, cfAllocator *alloc);

#define IMAGE_H
#endif
