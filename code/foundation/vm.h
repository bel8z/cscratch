#ifndef CF_VM_H

#include "common.h"

// Platform provided virtual memory API

#define VM_RESERVE_FUNC(name) void *name(usize size)
#define VM_RELEASE_FUNC(name) void name(void *memory, usize size)

#define VM_COMMIT_FUNC(name) bool name(void *memory, usize size)
#define VM_REVERT_FUNC(name) void name(void *memory, usize size)

typedef struct cfVirtualMemory
{
    VM_RESERVE_FUNC((*reserve));
    VM_RELEASE_FUNC((*release));
    VM_COMMIT_FUNC((*commit));
    VM_REVERT_FUNC((*revert));

    usize page_size;
} cfVirtualMemory;

#define cfVmReserve(vm, size) vm->reserve(size)
#define cfVmRelease(vm, mem, size) vm->release(mem, size)
#define cfVmCommit(vm, mem, size) vm->commit(mem, size)
#define cfVmRevert(vm, mem, size) vm->revert(mem, size)

#define CF_VM_H
#endif
