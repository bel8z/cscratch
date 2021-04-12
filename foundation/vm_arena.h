#ifndef VM_ARENA_H

#include "common.h"

/// Linear arena allocator based on a block of virtual memory
typedef struct VmArena
{
    u32 reserved;
    u32 committed;
    u32 allocated;
    u32 page_size;
    u8 *memory;
} VmArena;

/// Initialize the arena by reserving a block of virtual memory of the required
/// size
bool arena_init(VmArena *arena, u32 reserved_size);
/// Free all the memory allocated by the arena. The arena is unable to provide
/// any memory after this call.
void arena_free(VmArena *arena);
/// Decommit (return to the OS) the amount of reserved memory that is no more in
/// use
void arena_shrink(VmArena *arena);

/// Retrieves a block of the given size from the top of the arena stack
void *arena_push_size(VmArena *arena, u32 size);
/// Returns a block of the given size to the top of the arena stack; this is
/// effective only if the block matches with the last allocation
void arena_pop_size(VmArena *arena, u32 size, void const *memory);

/// Pushes a struct on the arena stack
#define arena_push_struct(arena, Type)                                         \
    (Type *)arena_push_size(arena, sizeof(Type))

/// Pushes an array of structs on the arena stack
#define arena_push_array(arena, Type, count)                                   \
    (Type *)arena_push_size(arena, count * sizeof(Type))

/// Pops a struct from the arena stack; this is effective only if the struct
/// matches with the last allocation
#define arena_pop_struct(arena, Type, ptr)                                     \
    arena_pop_size(arena, sizeof(Type), ptr)

/// Pops an array of structs from the arena stack; this is effective only if the
/// array matches with the last allocation
#define arena_pop_array(arena, Type, count, ptr)                               \
    arena_pop_size(arena, count * sizeof(Type), ptr)

#define VM_ARENA_H
#endif
