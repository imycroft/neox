#pragma once

#include "fs_types.h"

#define FS_MAGIC        0x4E584653U
#define FS_VERSION      1U
#define FS_FILENAME_MAX 64U

struct fs_superblock {
    fs_uint32_t magic;
    fs_uint32_t version;
    fs_uint32_t file_count;
    fs_uint32_t files_offset;
    fs_uint32_t data_offset;
};

struct fs_file {
    char        name[FS_FILENAME_MAX];
    fs_uint32_t offset;
    fs_uint32_t size;
};

_Static_assert(sizeof(struct fs_superblock) == 20U,
               "fs_superblock has invalid size");

_Static_assert(sizeof(struct fs_file) == 72U,
               "fs_file has invalid size");
