#pragma once

#include "foundation/core.h"

// TODO (Matteo):
// * Keep image bits around (for querying/manipulation)?
// * Implement image view navigation (scale/offset)
// * Implement a file queue for browsing
// * Implement image rotation

typedef struct Image
{
    union
    {
        Rgba32 *pixels;
        U8 *bytes;
    };
    I32 width;
    I32 height;
} Image;

bool imageLoadFromFile(Image *image, const char *filename);
bool imageLoadFromMemory(Image *image, U8 const *in_data, Usize in_data_size);
void imageUnload(Image *image);
