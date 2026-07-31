#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "thread.h"

void scheduler_init(void);

void scheduler_add(struct thread *thread);

struct thread *scheduler_current(void);

struct thread *scheduler_next(void);

#endif
