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
    u32 texture;
    i32 width;
    i32 height;
    f32 zoom;
    ImageFilter filter;
} Image;

// Simple helper functions to load an image into a OpenGL texture with common settings
bool imageLoadFromFile(Image *image, const char *filename, cfAllocator *alloc);
bool imageLoadFromMemory(Image *image, u8 const *in_data, usize in_data_size, cfAllocator *alloc);

void imageSetFilter(Image *image, ImageFilter filter);

void imageUnload(Image *image);

#define IMAGE_H
#endif
