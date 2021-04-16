#ifndef VM_ARENA_H

#include "common.h"
#include "vm.h"

/// Linear arena allocator based on a block of virtual memory
typedef struct VmArena
{
    u32 reserved;
    u32 committed;
    u32 allocated;
    u8 *memory;
    cfVirtualMemory *vm;
} VmArena;

/// Initialize the arena by reserving a block of virtual memory of the required
/// size
bool arenaInit(VmArena *arena, cfVirtualMemory *vm, u32 reserved_size);
/// Free all the memory allocated by the arena. The arena is unable to provide
/// any memory after this call.
void arenaFree(VmArena *arena);
/// Decommit (return to the OS) the amount of reserved memory that is no more in
/// use
void arenaShrink(VmArena *arena);

/// Retrieves a block of the given size from the top of the arena stack
void *arenaPushSize(VmArena *arena, u32 size);
/// Returns a block of the given size to the top of the arena stack; this is
/// effective only if the block matches with the last allocation
void arenaPopSize(VmArena *arena, u32 size, void const *memory);

/// Pushes a struct on the arena stack
#define arenaPushStruct(arena, Type) (Type *)arenaPushSize(arena, sizeof(Type))

/// Pushes an array of structs on the arena stack
#define arenaPushArray(arena, Type, count) (Type *)arenaPushSize(arena, count * sizeof(Type))

/// Pops a struct from the arena stack; this is effective only if the struct
/// matches with the last allocation
#define arenaPopStruct(arena, Type, ptr) arenaPopSize(arena, sizeof(Type), ptr)

/// Pops an array of structs from the arena stack; this is effective only if the
/// array matches with the last allocation
#define arenaPopArray(arena, Type, count, ptr) arenaPopSize(arena, count * sizeof(Type), ptr)

#define VM_ARENA_H
#endif
