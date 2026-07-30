#pragma once

#include "types.h"

void pit_init(uint32_t frequency);

void pit_handler(void);

uint32_t pit_get_ticks(void);

void pit_sleep(uint64_t ticks);

uint32_t pit_get_frequency(void);
