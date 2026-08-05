#pragma once

/*
 * Internal scheduler interface.
 *
 * This header is intended only for scheduler-related kernel
 * subsystems (thread, wait queue, etc.). It must not be
 * included by arbitrary kernel code.
 */

void scheduler_yield(void);
