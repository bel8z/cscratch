#pragma once

#include "foundation/core.h"

//------------------------------------------------------------------------------
// Platform interface
//------------------------------------------------------------------------------

// Foundation interfaces

typedef struct CfVirtualMemory CfVirtualMemory;
typedef struct CfFileSystem CfFileSystem;
typedef struct CfTimeApi CfTimeApi;

// Dynamic loading interface

typedef struct Library Library;

typedef struct LibraryApi
{
    Library *(*load)(Str filename);
    void (*unload)(Library *lib);
    void *(*loadSymbol)(Library *restrict lib, Cstr restrict name);
} LibraryApi;

// Common program paths

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

// Optional platform interfaces for graphical applications

typedef struct GlApi GlApi;
typedef struct Gui Gui;

/// Main platform interface
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

    /// File system services
    CfFileSystem *fs;

    /// System time services
    CfTimeApi *time;

    /// Dynamic library loading
    LibraryApi *library;

    /// Common program paths
    Paths *paths;

    /// OpenGL API [optional]
    GlApi *gl;

    // Dear Imgui state [optional]
    Gui *gui;

} Platform;
