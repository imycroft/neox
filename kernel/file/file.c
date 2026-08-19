#include "file.h"

#include "fs.h"
#include "string.h"

int file_open(
    const char *path,
    uint32_t access,
    struct file *file
)
{
    if (access != FILE_ACCESS_READ &&
        access != FILE_ACCESS_WRITE &&
        access != FILE_ACCESS_READ_WRITE)
    {
        return -1;
    }
    const struct fs_file *fs_file;

    if (path == NULL)
        return -1;

    if (file == NULL)
        return -1;

    fs_file = fs_find(path);

    if (fs_file == NULL)
        return -1;

    file->fs_file = fs_file;
    file->position = 0;
    file->access = access;

    return 0;
}

const void *file_data(const struct file *file)
{
    if (file == NULL)
        return NULL;

    return fs_file_data(file->fs_file);
}

uint32_t file_size(const struct file *file)
{
    if (file == NULL || file->fs_file == NULL)
        return 0;

    return fs_file_size(file->fs_file);
}

ssize_t file_read(
    struct file *file,
    void *buffer,
    size_t count
)
{

    uint32_t remaining;

    if (file == NULL)
        return -1;

    if (buffer == NULL)
        return -1;

    if (file->fs_file == NULL)
        return -1;

    if (file->access != FILE_ACCESS_READ &&
        file->access != FILE_ACCESS_READ_WRITE)
    {
        return -1;
    }

    if (file->position >= file_size(file))
        return 0;

    remaining = file_size(file) - file->position;

    if (count > remaining)
        count = remaining;

    memcpy(
        buffer,
        (const uint8_t *)file_data(file) + file->position,
           count
    );

    file->position += count;

    return (ssize_t)count;
}

void file_close(struct file *file)
{
    if (file == NULL)
        return;

    file->fs_file = NULL;
    file->position = 0;
    file->access = 0;
}
