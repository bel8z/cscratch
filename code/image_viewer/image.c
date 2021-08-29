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

// Custom IO operations for stbi
static CfFileSystem *g_fs = NULL;

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
    Usize total_size = size + sizeof(size);
    Usize *buffer = memAlloc(g_alloc, total_size);

    if (buffer) *(buffer++) = total_size;

    return buffer;
}

void *
stbiRealloc(void *memory, Usize size)
{
    Usize *buffer = memory;

    if (buffer)
    {
        Usize total_size = size + sizeof(size);
        buffer -= 1;
        buffer = memRealloc(g_alloc, buffer, *buffer, total_size);

        if (buffer) *(buffer++) = total_size;
    }

    return buffer;
}

void
stbiFree(void *memory)
{
    if (memory)
    {
        Usize *buffer = memory;
        buffer -= 1;
        memFree(g_alloc, buffer, *buffer);
    }
}

//------------------------------------------------------------------------------
// IO operations

static I32
stbiRead(void *user, Char8 *data, I32 size)
{
    FileStream *file = user;
    return (I32)g_fs->fileStreamRead(file, (U8 *)data, (Usize)size);
}

static void
stbiSkip(void *user, I32 n)
{
    FileStream *file = user;
    g_fs->fileStreamSeek(file, FileSeekPos_Current, (Usize)n);
}

static I32
stbiEof(void *user)
{
    FileStream *file = user;
    return file->flags & FileStreamFlags_Eof;
}

//------------------------------------------------------------------------------
// Image API implementation

void
imageInit(MemAllocator alloc, CfFileSystem *fs)
{
    CF_ASSERT_NOT_NULL(fs);
    g_alloc = alloc;
    g_fs = fs;
}

bool
imageLoadFromFile(Image *image, Cstr filename)
{
    CF_ASSERT_NOT_NULL(image);
    CF_ASSERT_NOT_NULL(filename);
    CF_ASSERT(!image->bytes, "overwriting valid image");

    FileStream file = g_fs->fileStreamOpen(strFromCstr(filename), FileOpenMode_Read);
    stbi_io_callbacks cb = {.eof = stbiEof, .read = stbiRead, .skip = stbiSkip};

    image->width = 0;
    image->height = 0;
    image->bytes = stbi_load_from_callbacks(&cb, &file, &image->width, &image->height, NULL, 4);

    g_fs->fileStreamClose(&file);

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
