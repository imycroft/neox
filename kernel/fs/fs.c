#include "fs.h"

#include "fs_format.h"
#include "types.h"
#include "string.h"

static const uint8_t *fs_image;
static uint32_t fs_image_size;
static const struct fs_superblock *fs_superblock;

int fs_mount(const void *image, uint32_t size)
{
    uint32_t files_end;

    if (image == NULL)
        return -1;

    if (size < sizeof(struct fs_superblock))
        return -1;

    fs_image = (const uint8_t *)image;
    fs_image_size = size;

    fs_superblock = (const struct fs_superblock *)fs_image;

    if (fs_superblock->magic != FS_MAGIC)
        return -1;

    if (fs_superblock->version != FS_VERSION)
        return -1;

    /*
     * The file table must fit completely inside the image.
     */
    if (fs_superblock->files_offset > size)
        return -1;

    if (fs_superblock->file_count >
        (size - fs_superblock->files_offset) / sizeof(struct fs_file))
        return -1;

    files_end = fs_superblock->files_offset +
    fs_superblock->file_count * sizeof(struct fs_file);

    /*
     * File data must begin after the file table.
     */
    if (fs_superblock->data_offset < files_end)
        return -1;

    if (fs_superblock->data_offset > size)
        return -1;

    return 0;
}

const struct fs_file *fs_find(const char *path)
{
    const struct fs_file *files;
    uint32_t i;

    if (fs_superblock == NULL)
        return NULL;

    files = (const struct fs_file *)
    (fs_image + fs_superblock->files_offset);

    for (i = 0; i < fs_superblock->file_count; i++)
    {
        if (strcmp(files[i].name, path) == 0)
            return &files[i];
    }

    return NULL;
}

const void *fs_file_data(const struct fs_file *file)
{
    if (file == NULL)
        return NULL;

    if (file->offset > fs_image_size)
        return NULL;

    if (file->size > fs_image_size - file->offset)
        return NULL;

    return fs_image + file->offset;
}

uint32_t fs_file_size(const struct fs_file *file)
{
    if (file == NULL)
        return 0;

    return file->size;
}
