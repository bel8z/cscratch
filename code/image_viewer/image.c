#include "image.h"

#include "foundation/allocator.h"

#include "gl/gload.h"

//------------------------------------------------------------------------------
// Custom memory management for stbi
static cfAllocator *g_alloc = NULL;
static U32 g_loaded = 0;

static void *stbiRealloc(void *mem, Usize size);
static void stbiFree(void *mem);

#define STBI_MALLOC(size) stbiRealloc(NULL, size)
#define STBI_REALLOC(mem, size) stbiRealloc(mem, size)
#define STBI_FREE(mem) stbiFree(mem)

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
imageLoadFromFile(Image *image, const char *filename, cfAllocator *alloc)
{
    CF_ASSERT_NOT_NULL(alloc);
    CF_ASSERT_NOT_NULL(image);
    CF_ASSERT_NOT_NULL(filename);

    CF_ASSERT(!image->data, "overwriting valid image");

    // Setup allocator for stbi
    g_alloc = alloc;

    // Load from file
    image->width = 0;
    image->height = 0;
    U8 *data = stbi_load(filename, &image->width, &image->height, NULL, 4);

    g_alloc = NULL;

    if (data)
    {
        g_loaded++;
        image->data = (Rgba32 *)data;
        return true;
    }

    return false;
}

bool
imageLoadFromMemory(Image *image, U8 const *in_data, Usize in_data_size, cfAllocator *alloc)
{
    CF_ASSERT_NOT_NULL(alloc);
    CF_ASSERT_NOT_NULL(image);
    CF_ASSERT_NOT_NULL(in_data);

    // Setup allocator for stbi
    g_alloc = alloc;

    image->width = 0;
    image->height = 0;
    U8 *data =
        stbi_load_from_memory(in_data, (I32)in_data_size, &image->width, &image->height, NULL, 4);

    g_alloc = NULL;

    if (data)
    {
        g_loaded++;
        image->data = (Rgba32 *)data;
        return true;
    }

    return false;
}

void
imageUnload(Image *image, cfAllocator *alloc)
{
    CF_ASSERT_NOT_NULL(image);
    CF_ASSERT_NOT_NULL(alloc);

    g_alloc = alloc;

    if (image->data)
    {
        g_loaded--;
        stbiFree(image->data);
    }

    image->data = NULL;
    image->height = 0;
    image->width = 0;

    g_alloc = NULL;
}

ImageStats
imageGetStats(void)
{
    return (ImageStats){.loaded = g_loaded};
}

//------------------------------------------------------------------------------
// stbi memory management

void *
stbiRealloc(void *mem, Usize size)
{
    CF_ASSERT_NOT_NULL(g_alloc);
    CF_ASSERT(size > 0, "stbi requested allocation of 0 bytes");
    CF_ASSERT(size < USIZE_MAX - sizeof(Usize), "stbi requested allocation is too big");

    Usize *old_buf = mem;
    Usize old_size = 0;

    if (old_buf)
    {
        --old_buf;
        old_size = *old_buf;
    }

    Usize new_size = size + sizeof(*old_buf);
    Usize *new_buf = cfRealloc(g_alloc, old_buf, old_size, new_size);

    if (!new_buf) return NULL;

    *new_buf = new_size;

    return new_buf + 1;
}

void
stbiFree(void *mem)
{
    CF_ASSERT_NOT_NULL(g_alloc);

    if (mem)
    {
        Usize *buf = (Usize *)mem - 1;
        Usize old_size = *buf;
        CF_ASSERT(old_size, "stbi freeing invalid block");
        cfFree(g_alloc, buf, old_size);
    }
}

//------------------------------------------------------------------------------
