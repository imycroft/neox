#pragma once

#include "types.h"

struct fs_file;

int fs_mount(const void *image, uint32_t size);

const struct fs_file *fs_find(const char *path);

const void *fs_file_data(const struct fs_file *file);

uint32_t fs_file_size(const struct fs_file *file);
