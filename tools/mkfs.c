#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <fs_format.h>

struct input_file {
    const char *name;
    const char *host_path;
    fs_uint32_t size;
    fs_uint32_t offset;
};

static void die(const char *message)
{
    fprintf(stderr, "mkfs: %s\n", message);
    exit(EXIT_FAILURE);
}

static fs_uint32_t get_file_size(const char *path)
{
    FILE *file = fopen(path, "rb");

    if (file == NULL) {
        perror(path);
        exit(EXIT_FAILURE);
    }

    if (fseek(file, 0, SEEK_END) != 0) {
        fclose(file);
        die("failed to seek to end of file");
    }

    long size = ftell(file);

    if (size < 0) {
        fclose(file);
        die("failed to determine file size");
    }

    if ((unsigned long)size > 0xFFFFFFFFUL) {
        fclose(file);
        die("input file is too large");
    }

    fclose(file);

    return (fs_uint32_t)size;
}

static void copy_file(FILE *destination,
                      const char *source_path,
                      fs_uint32_t size)
{
    FILE *source = fopen(source_path, "rb");

    if (source == NULL) {
        perror(source_path);
        exit(EXIT_FAILURE);
    }

    unsigned char buffer[4096];
    fs_uint32_t remaining = size;

    while (remaining != 0U) {
        fs_uint32_t chunk = remaining;

        if (chunk > sizeof(buffer)) {
            chunk = sizeof(buffer);
        }

        size_t count = fread(buffer, 1, chunk, source);

        if (count != chunk) {
            fclose(source);
            die("failed to read input file");
        }

        count = fwrite(buffer, 1, chunk, destination);

        if (count != chunk) {
            fclose(source);
            die("failed to write filesystem image");
        }

        remaining -= chunk;
    }

    fclose(source);
}

static void create_file_entry(struct fs_file *entry,
                              const struct input_file *input)
{
    size_t length = strlen(input->name);

    if (length >= FS_FILENAME_MAX) {
        die("filesystem path is too long");
    }

    memset(entry, 0, sizeof(*entry));
    memcpy(entry->name, input->name, length);

    entry->offset = input->offset;
    entry->size = input->size;
}

static void parse_input(struct input_file *input, const char *argument)
{
    const char *separator = strchr(argument, '=');

    if (separator == NULL) {
        die("input must have the form <path>=<source>");
    }

    size_t name_length = (size_t)(separator - argument);

    if (name_length == 0U) {
        die("filesystem path cannot be empty");
    }

    if (name_length >= FS_FILENAME_MAX) {
        die("filesystem path is too long");
    }

    char *name = malloc(name_length + 1U);

    if (name == NULL) {
        die("out of memory");
    }

    memcpy(name, argument, name_length);
    name[name_length] = '\0';

    input->name = name;
    input->host_path = separator + 1;
    input->size = get_file_size(input->host_path);
}

int main(int argc, char **argv)
{
    if (argc < 3) {
        fprintf(stderr,
                "usage: %s <output> <path=source> [<path=source> ...]\n",
                argv[0]);

        return EXIT_FAILURE;
    }

    int input_count = argc - 2;

    struct input_file *inputs =
    calloc((size_t)input_count, sizeof(*inputs));

    if (inputs == NULL) {
        die("out of memory");
    }

    for (int i = 0; i < input_count; ++i) {
        parse_input(&inputs[i], argv[i + 2]);
    }

    if ((unsigned long)input_count > 0xFFFFFFFFUL) {
        die("too many filesystem files");
    }

    fs_uint32_t file_count = (fs_uint32_t)input_count;

    fs_uint32_t files_offset =
    sizeof(struct fs_superblock);

    fs_uint32_t data_offset =
    files_offset +
    file_count * sizeof(struct fs_file);

    fs_uint32_t current_offset = data_offset;

    for (int i = 0; i < input_count; ++i) {
        inputs[i].offset = current_offset;

        current_offset += inputs[i].size;
    }

    struct fs_superblock superblock = {
        .magic = FS_MAGIC,
        .version = FS_VERSION,
        .file_count = file_count,
        .files_offset = files_offset,
        .data_offset = data_offset
    };

    struct fs_file *files =
    calloc((size_t)input_count, sizeof(*files));

    if (files == NULL) {
        die("out of memory");
    }

    for (int i = 0; i < input_count; ++i) {
        create_file_entry(&files[i], &inputs[i]);
    }

    const char *output_path = argv[1];

    FILE *output = fopen(output_path, "wb");

    if (output == NULL) {
        perror(output_path);
        return EXIT_FAILURE;
    }

    if (fwrite(&superblock,
        sizeof(superblock),
               1,
               output) != 1) {
        fclose(output);
    die("failed to write superblock");
               }

               if (fwrite(files,
                   sizeof(*files),
                          (size_t)input_count,
                          output) != (size_t)input_count) {
                   fclose(output);
               die("failed to write file table");
                          }

                          for (int i = 0; i < input_count; ++i) {
                              copy_file(output,
                                        inputs[i].host_path,
                                        inputs[i].size);
                          }

                          fclose(output);

                          printf("Created %s\n", output_path);
                          printf("Files: %u\n", file_count);

                          for (int i = 0; i < input_count; ++i) {
                              printf("  %s: offset=%u size=%u\n",
                                     inputs[i].name,
                                     inputs[i].offset,
                                     inputs[i].size);
                          }

                          for (int i = 0; i < input_count; ++i) {
                              free((void *)inputs[i].name);
                          }

                          free(files);
                          free(inputs);

                          return EXIT_SUCCESS;
}
