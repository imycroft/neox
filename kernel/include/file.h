#pragma once

#include "types.h"

#define FILE_ACCESS_READ       0x0001u
#define FILE_ACCESS_WRITE      0x0002u
#define FILE_ACCESS_READ_WRITE (FILE_ACCESS_READ | FILE_ACCESS_WRITE)

struct fs_file;

struct file
{
    const struct fs_file *fs_file;
    uint32_t position;
    uint32_t access;
};

int file_open(
    const char *path,
    uint32_t access,
    struct file *file
);

const void *file_data(const struct file *file);

uint32_t file_size(const struct file *file);

ssize_t file_read(
    struct file *file,
    void *buffer,
    size_t count
);

void file_close(struct file *file);
