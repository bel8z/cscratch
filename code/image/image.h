#ifndef IMAGE_H

#include "foundation/common.h"

// TODO (Matteo):
// * Keep image bits around (for querying/manipulation)?
// * Implement image view navigation (scale/offset)
// * Implement a file queue for browsing
// * Implement image rotation

typedef enum ImageFilter
{
    ImageFilter_Nearest,
    ImageFilter_Linear,
} ImageFilter;

typedef struct Image
{
    U32 texture;
    I32 width;
    I32 height;
} Image;

typedef struct GlApi GlApi;
bool imageInit(GlApi *api);

// Simple helper functions to load an image into a OpenGL texture with common settings
bool imageLoadFromFile(Image *image, const char *filename, cfAllocator *alloc);
bool imageLoadFromMemory(Image *image, U8 const *in_data, Usize in_data_size, cfAllocator *alloc);

void imageSetFilter(Image *image, ImageFilter filter);

void imageUnload(Image *image);

#define IMAGE_H
#endif
