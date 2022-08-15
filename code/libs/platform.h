#pragma once

#include "foundation/core.h"
#include "foundation/time.h"

//------------------------------------------------------------------------------
// Platform interface
//------------------------------------------------------------------------------

//=== Foundation interfaces ===//

typedef struct VMemApi VMemApi;
typedef struct IoFileApi IoFileApi;

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
typedef struct ImGuiContext GuiContext;

//=== Main platform interface ===//

typedef struct Platform
{
    /// Virtual memory services
    VMemApi *vmem;
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
    IoFileApi *file;

    /// Tracks elapsed time since the start of the application (useful for performance measurements)
    Clock clock;

    /// Common program paths
    Paths *paths;

    /// OpenGL API [optional]
    GlApi *gl;

    // Dear Imgui state [optional]
    GuiContext *gui;

} Platform;
