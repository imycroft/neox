#pragma once

#include "types.h"

struct process;

int elf_validate(const void *image, uint32_t size);

void elf_dump_load_segments(const void *image);

int elf_load(struct process *process,
             const void *image,
             uint32_t size,
             uintptr_t *entry);
