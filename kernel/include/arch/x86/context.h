#ifndef CONTEXT_H
#define CONTEXT_H

#include "types.h"

/*
 * CPU context saved on a thread's kernel stack.
 *
 * The instruction pointer is restored by RET and
 * therefore is not part of this structure.
 */
struct cpu_context
{
    uint32_t edi;
    uint32_t esi;
    uint32_t ebx;
    uint32_t ebp;
};

/*
 * Save the current thread context and restore another.
 */
void context_switch(uintptr_t *old_sp,
                    uintptr_t new_sp);

#endif
