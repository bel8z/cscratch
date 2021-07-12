#include "image.h"

#include "gl/gload.h"

#include "foundation/memory.h"

// NOTE (Matteo): On memory allocation
//
// stbi_image allows for plugging in custom memory allocation functions via the STBI_MALLOC,
// STBI_REALLOC and STB_FREE macros.
// These are expected to match the corresponding stdlib functions, so a context parameter cannot be
// passed directly but a global is required instead. Obviously this is bad for concurrency.
//
// The behavior of stbi_image in this regard turned out to be reliable, and no temporary allocations
// have ever leaked.
//
// For now I choose to let it use the stdlib allocator and keeping it responsible for allocating and
// freeing images, until I come up with a solution that can satisfy dependencies which have this
// kind of API.

// Custom assertions for stbi
#define STBI_ASSERT(x) CF_ASSERT(x, "stb image assert")

// Include stbi implementation
#if CF_COMPILER_CLANG
#    pragma clang diagnostic push
#    pragma clang diagnostic ignored "-Wsign-conversion"
#    pragma clang diagnostic ignored "-Wsign-compare"
#    pragma clang diagnostic ignored "-Wimplicit-int-conversion"
#    pragma clang diagnostic ignored "-Wdouble-promotion"
#    pragma clang diagnostic ignored "-Wcast-align"
#endif

#define STB_IMAGE_IMPLEMENTATION

#include <stb_image.h>

#if CF_COMPILER_CLANG
#    pragma clang diagnostic pop
#endif

//------------------------------------------------------------------------------
// Image API implementation

bool
imageLoadFromFile(Image *image, const char *filename)
{
    CF_ASSERT_NOT_NULL(image);
    CF_ASSERT_NOT_NULL(filename);
    CF_ASSERT(!image->bytes, "overwriting valid image");

    image->width = 0;
    image->height = 0;
    image->bytes = stbi_load(filename, &image->width, &image->height, NULL, 4);

    return (image->bytes != NULL);
}

bool
imageLoadFromMemory(Image *image, U8 const *in_data, Usize in_data_size)
{
    CF_ASSERT_NOT_NULL(image);
    CF_ASSERT_NOT_NULL(in_data);
    CF_ASSERT(!image->bytes, "overwriting valid image");

    image->width = 0;
    image->height = 0;
    image->bytes =
        stbi_load_from_memory(in_data, (I32)in_data_size, &image->width, &image->height, NULL, 4);

    return (image->bytes != NULL);
}

void
imageUnload(Image *image)
{
    CF_ASSERT_NOT_NULL(image);

    stbi_image_free(image->bytes);

    image->bytes = NULL;
    image->height = 0;
    image->width = 0;
}

//------------------------------------------------------------------------------
