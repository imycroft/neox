#ifndef ARCH_H
#define ARCH_H

#include "types.h"

typedef uint32_t interrupt_state_t;

interrupt_state_t interrupt_save(void);

void interrupt_restore(interrupt_state_t state);

void interrupt_enable(void);

void interrupt_disable(void);

bool interrupt_enabled(void);

void arch_init(void);

#endif
