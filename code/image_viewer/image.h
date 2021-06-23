#pragma once

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
    Rgba32 *data;
    I32 width;
    I32 height;
} Image;

bool imageLoadFromFile(Image *image, const char *filename, cfAllocator *alloc);
bool imageLoadFromMemory(Image *image, U8 const *in_data, Usize in_data_size, cfAllocator *alloc);
void imageUnload(Image *image, cfAllocator *alloc);
