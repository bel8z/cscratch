#include "image.h"

#include "gl/gload.h"

//------------------------------------------------------------------------------

// NOTE (Matteo) On memory allocation
// stbi_image allows for plugging in custom memory allocation functions via the STBI_MALLOC,
// STBI_REALLOC and STB_FREE macros.
// These are expected to match the corresponding stdlib functions, so a context parameter cannot be
// passed directly but a global is required instead. Obviously this is bad for concurrency.
// Since i trust the library quite a bit, I let it use standard allocation functions for its
// temporary buffers, and then copy the final image in a custom allocation. In this way I still have
// control of the memory I use on the application side, and temporary allocations are transparent
// (not great, but still better then a global which requires synchronization - malloc is already
// synchronized)
//

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

#define imageSize(image) (Usize)(4 * image->width * image->height)

bool
imageLoadFromFile(Image *image, const char *filename, cfAllocator alloc)
{
    CF_ASSERT_NOT_NULL(image);
    CF_ASSERT_NOT_NULL(filename);
    CF_ASSERT(!image->bytes, "overwriting valid image");

    image->width = 0;
    image->height = 0;
    U8 *data = stbi_load(filename, &image->width, &image->height, NULL, 4);

    if (data)
    {
        Usize size = imageSize(image);
        image->bytes = cfAlloc(&alloc, size);
        if (image->bytes) cfMemCopy(data, image->bytes, size);
        stbi_image_free(data);
    }

    return (image->bytes != NULL);
}

bool
imageLoadFromMemory(Image *image, U8 const *in_data, Usize in_data_size, cfAllocator alloc)
{
    CF_ASSERT_NOT_NULL(image);
    CF_ASSERT_NOT_NULL(in_data);
    CF_ASSERT(!image->bytes, "overwriting valid image");

    image->width = 0;
    image->height = 0;
    U8 *data =
        stbi_load_from_memory(in_data, (I32)in_data_size, &image->width, &image->height, NULL, 4);

    if (data)
    {
        Usize size = imageSize(image);
        image->bytes = cfAlloc(&alloc, size);
        if (image->bytes) cfMemCopy(data, image->bytes, size);
        stbi_image_free(data);
    }

    return (image->bytes != NULL);
}

void
imageUnload(Image *image, cfAllocator alloc)
{
    CF_ASSERT_NOT_NULL(image);

    cfFree(&alloc, image->bytes, imageSize(image));

    image->bytes = NULL;
    image->height = 0;
    image->width = 0;
}

//------------------------------------------------------------------------------
