#include "image.h"

#include "foundation/fs.h"
#include "foundation/memory.h"
#include "foundation/strings.h"

// NOTE (Matteo): On memory allocation
//
// stbi_image allows for plugging in custom memory allocation functions via the STBI_MALLOC,
// STBI_REALLOC and STB_FREE macros.
// These are expected to match the corresponding stdlib functions, so a context parameter cannot be
// passed directly but a global is required instead.
// Obviously this is bad for concurrency, but I worked around it by setting the global allocator at
// application load.

// Custom assertions for stbi
#define STBI_ASSERT(x) CF_ASSERT(x, "stb image assert")

// Custom memory management for stbi
static MemAllocator g_alloc;
static void *stbiAlloc(Usize size);
static void *stbiRealloc(void *memory, Usize size);
static void stbiFree(void *memory);

#define STBI_MALLOC stbiAlloc
#define STBI_REALLOC stbiRealloc
#define STBI_FREE stbiFree

#define STBI_NO_STDIO

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
// Memory management

void *
stbiAlloc(Usize size)
{
    Usize total_size = size + sizeof(total_size);
    Usize *buffer = memAlloc(g_alloc, total_size);

    if (buffer)
    {
        *buffer = total_size;
        ++buffer;
    }

    return buffer;
}

void *
stbiRealloc(void *memory, Usize size)
{
    Usize new_size = size + sizeof(new_size);
    Usize old_size = 0;
    Usize *old_buffer = memory;

    if (old_buffer)
    {
        old_buffer--;
        old_size = *old_buffer;
    }

    Usize *new_buffer = memRealloc(g_alloc, old_buffer, old_size, new_size);

    if (new_buffer)
    {
        *new_buffer = new_size;
        ++new_buffer;
    }

    return new_buffer;
}

void
stbiFree(void *memory)
{
    if (memory)
    {
        Usize *buffer = memory;
        buffer--;
        memFree(g_alloc, buffer, *buffer);
    }
}

//------------------------------------------------------------------------------
// IO operations

static I32
stbiRead(void *user, Char8 *data, I32 size)
{
    File *file = user;
    return (I32)fileRead(file, (U8 *)data, (Usize)size);
}

static void
stbiSkip(void *user, I32 n)
{
    File *file = user;
    fileSeek(file, FileSeekPos_Current, (Usize)n);
}

static I32
stbiEof(void *user)
{
    File *file = user;
    return file->eof;
}

//------------------------------------------------------------------------------
// Image API implementation

void
imageInit(MemAllocator alloc)
{
    g_alloc = alloc;
}

bool
imageLoadFromFile(Image *image, Cstr filename)
{
    CF_ASSERT_NOT_NULL(image);
    CF_ASSERT_NOT_NULL(filename);
    CF_ASSERT(!image->bytes, "overwriting valid image");

    File file = fileOpen(strFromCstr(filename), FileOpenMode_Read);
    stbi_io_callbacks cb = {.eof = stbiEof, .read = stbiRead, .skip = stbiSkip};

    image->width = 0;
    image->height = 0;
    image->bytes = stbi_load_from_callbacks(&cb, &file, &image->width, &image->height, NULL, 4);

    fileClose(&file);

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
