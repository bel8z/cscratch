#ifndef ALLOCATOR_H

#include <stddef.h>

typedef struct Allocator
{
    void *state;
    void *(*allocate)(size_t size, void *state);
    void *(*reallocate)(void *memory, size_t size, void *state);
    void (*deallocate)(void *memory, void *state);

} Allocator;

#define ALLOCATOR_H
#endif
