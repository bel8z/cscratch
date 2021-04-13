#ifndef CF_VM_H

#include "common.h"

#define VM_RESERVE_FUNC(name) void *name(void *state, usize size)
#define VM_RELEASE_FUNC(name) void name(void *state, void *memory, usize size)

#define VM_COMMIT_FUNC(name) bool name(void *state, void *memory, usize size)
#define VM_REVERT_FUNC(name) void name(void *state, void *memory, usize size)

typedef VM_RESERVE_FUNC(VmReserveFunc);
typedef VM_RELEASE_FUNC(VmReleaseFunc);
typedef VM_COMMIT_FUNC(VmCommitFunc);
typedef VM_REVERT_FUNC(VmRevertFunc);

typedef struct Vm
{
    void *state; // Optional

    VmReserveFunc *reserve;
    VmReleaseFunc *release;

    VmCommitFunc *commit;
    VmRevertFunc *revert;

    usize page_size;
} Vm;

#define VM_RESERVE(vm, size) vm->reserve(vm->state, size)
#define VM_RELEASE(vm, mem, size) vm->release(vm->state, mem, size)

#define VM_COMMIT(vm, mem, size) vm->commit(vm->state, mem, size)
#define VM_REVERT(vm, mem, size) vm->revert(vm->state, mem, size)

#define CF_VM_H
#endif
