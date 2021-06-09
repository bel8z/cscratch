#include "image.h"

#include "foundation/allocator.h"

#include "gl/gload.h"

//------------------------------------------------------------------------------
// Custom memory management for stbi
static cfAllocator *g_alloc = NULL;

static void *stbiRealloc(void *mem, Usize size);
static void stbiFree(void *mem);

#define STBI_MALLOC(size) stbiRealloc(NULL, size)
#define STBI_REALLOC(mem, size) stbiRealloc(mem, size)
#define STBI_FREE(mem) stbiFree(mem)

// Custom assertions for stbi
#define STBI_ASSERT(x) CF_ASSERT(x, "stb image assert")

// Include stbi implementation
#if CF_COMPILER_CLANG
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wsign-conversion"
#pragma clang diagnostic ignored "-Wsign-compare"
#pragma clang diagnostic ignored "-Wimplicit-int-conversion"
#pragma clang diagnostic ignored "-Wdouble-promotion"
#pragma clang diagnostic ignored "-Wcast-align"
#endif

#define STB_IMAGE_IMPLEMENTATION

#include <stb_image.h>

#if CF_COMPILER_CLANG
#pragma clang diagnostic pop
#endif

//------------------------------------------------------------------------------
// Image API implementation

static void
image__processData(U8 *data, I32 width, I32 height, Image *image)
{
    // Create a OpenGL texture identifier
    U32 image_texture;
    glGenTextures(1, &image_texture);
    glBindTexture(GL_TEXTURE_2D, image_texture);

    // TODO (Matteo): Separate texture parameterization from loading

    // Setup filtering parameters for display
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    // These are required on WebGL for non power-of-two textures
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // Upload pixels into texture
#if defined(GL_UNPACK_ROW_LENGTH) && !defined(__EMSCRIPTEN__)
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
#endif
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);

    image->texture = image_texture;
    image->width = width;
    image->height = height;
}

bool
imageLoadFromFile(Image *image, const char *filename, cfAllocator *alloc)
{
    CF_ASSERT_NOT_NULL(alloc);
    CF_ASSERT_NOT_NULL(image);
    CF_ASSERT_NOT_NULL(filename);

    // Setup allocator for stbi
    g_alloc = alloc;

    // Load from file
    I32 width = 0;
    I32 height = 0;
    U8 *data = stbi_load(filename, &width, &height, NULL, 4);
    bool result = false;

    if (data)
    {
        result = true;
        image__processData(data, width, height, image);
        stbi_image_free(data);
    }

    g_alloc = NULL;

    return result;
}

bool
imageLoadFromMemory(Image *image, U8 const *in_data, Usize in_data_size, cfAllocator *alloc)
{
    CF_ASSERT_NOT_NULL(alloc);
    CF_ASSERT_NOT_NULL(image);
    CF_ASSERT_NOT_NULL(in_data);

    // Setup allocator for stbi
    g_alloc = alloc;

    // Load from file
    I32 width = 0;
    I32 height = 0;
    U8 *data = stbi_load_from_memory(in_data, (I32)in_data_size, &width, &height, NULL, 4);
    bool result = false;

    if (data)
    {
        result = true;
        image__processData(data, width, height, image);
        stbi_image_free(data);
    }

    g_alloc = NULL;

    return result;
}

void
imageSetFilter(Image *image, ImageFilter filter)
{
    CF_ASSERT_NOT_NULL(image);

    glBindTexture(GL_TEXTURE_2D, image->texture);

    I32 value = (filter == ImageFilter_Linear) ? GL_LINEAR : GL_NEAREST;

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, value);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, value);
}

void
imageUnload(Image *image)
{
    glDeleteTextures(1, &image->texture);
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
