#pragma once

#include "foundation/core.h"
#include "foundation/time.h"

//------------------------------------------------------------------------------
// Platform interface
//------------------------------------------------------------------------------

//=== Foundation interfaces ===//

typedef struct CfVirtualMemory CfVirtualMemory;
typedef struct FileApi FileApi;

//=== Dynamic library loading ===//

typedef struct Library Library;

typedef struct LibraryApi
{
    Library *(*load)(Str filename);
    void (*unload)(Library *lib);
    void *(*loadSymbol)(Library *restrict lib, Cstr restrict name);
} LibraryApi;

//=== Common application paths ===//

enum
{
    Paths_Size = 256
};

// TODO (Matteo): Maybe simplify a bit? I suspect that the full exe path is enough
typedef struct Paths
{
    Char8 buffer[3 * Paths_Size];
    Str base, data, exe_name, lib_name;
} Paths;

//=== Command line arguments ===//

typedef struct CommandLine
{
    Usize len;
    Cstr *arg;
} CommandLine;

//=== Optional platform interfaces (gfx apps) ===//

typedef struct GlApi GlApi;
typedef struct Gui Gui;

//=== Main platform interface ===//

typedef struct Platform
{
    /// Virtual memory services
    CfVirtualMemory *vm;
    /// Reserved VM size in bytes
    // TODO (Matteo): Should be a pointer?
    Usize reserved_size;
    /// Committed VM size in bytes
    // TODO (Matteo): Should be a pointer?
    Usize committed_size;

    /// System heap allocator
    MemAllocator heap;
    /// Number of blocks allocated by the heap allocator
    // TODO (Matteo): Should be a pointer?
    Usize heap_blocks;
    // Total size in bytes of the allocation provided by the heap
    // TODO (Matteo): Should be a pointer?
    Usize heap_size;

    // File IO API
    FileApi *file;

    /// Tracks elapsed time since the start of the application (useful for performance measurements)
    Clock clock;

    /// Dynamic library loading
    LibraryApi *library;

    /// Common program paths
    Paths *paths;

    /// OpenGL API [optional]
    GlApi *gl;

    // Dear Imgui state [optional]
    Gui *gui;

} Platform;
