#include "image.h"

#include <GL/gl3w.h>

//------------------------------------------------------------------------------
// Custom memory management for stbi
static cfAllocator *g_alloc = NULL;

static void *stbiRealloc(void *mem, usize size);
static void stbiFree(void *mem);

#define STBI_MALLOC(size) stbiRealloc(NULL, size)
#define STBI_REALLOC(mem, size) stbiRealloc(mem, size)
#define STBI_FREE(mem) stbiFree(mem)

// Custom assertions for stbi
#define STBI_ASSERT(x) CF_ASSERT(x, "stb image assert")

// Include stbi implementation
#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wsign-conversion"
#pragma clang diagnostic ignored "-Wsign-compare"
#pragma clang diagnostic ignored "-Wimplicit-int-conversion"
#endif

#define STB_IMAGE_IMPLEMENTATION

#include "ext/stb/stb_image.h"

#if defined(__clang__)
#pragma clang diagnostic pop
#endif

//------------------------------------------------------------------------------
// Image API implementation

bool
imageLoadFromFile(Image *image, const char *filename, cfAllocator *alloc)
{
    CF_ASSERT_NOT_NULL(alloc);
    CF_ASSERT_NOT_NULL(image);
    CF_ASSERT_NOT_NULL(filename);

    // Setup allocator for stbi
    g_alloc = alloc;

    // Load from file
    i32 width = 0;
    i32 height = 0;
    u8 *data = stbi_load(filename, &width, &height, NULL, 4);
    if (!data) return false;

    // Create a OpenGL texture identifier
    u32 image_texture;
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
    stbi_image_free(data);

    image->texture = image_texture;
    image->width = width;
    image->height = height;
    image->filter = ImageFilter_Nearest;

    return true;
}

void
imageSetFilter(Image *image, ImageFilter filter)
{
    CF_ASSERT_NOT_NULL(image);

    if (filter != image->filter)
    {
        glBindTexture(GL_TEXTURE_2D, image->texture);

        i32 value = (filter == ImageFilter_Linear) ? GL_LINEAR : GL_NEAREST;

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, value);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, value);

        image->filter = filter;
    }
}

//------------------------------------------------------------------------------
// stbi memory management

void *
stbiRealloc(void *mem, usize size)
{
    if (!g_alloc) return NULL;

    usize *old_buf = mem;
    usize old_size = 0;

    if (old_buf)
    {
        --old_buf;
        old_size = *old_buf;
    }

    usize new_size = size + sizeof(*old_buf);
    usize *new_buf = CF_REALLOCATE(g_alloc, old_buf, old_size, new_size);

    if (new_buf) *(new_buf++) = size;

    return new_buf;
}

void
stbiFree(void *mem)
{
    if (g_alloc && mem)
    {
        usize *buf = mem;
        --buf;
        CF_DEALLOCATE(g_alloc, buf, *buf + sizeof(*buf));
    }
}

//------------------------------------------------------------------------------