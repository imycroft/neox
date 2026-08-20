#pragma once

#include "types.h"

struct page_directory;

#define EXEC_MAX_ARGS       32
#define EXEC_MAX_ARG_LENGTH 256

struct exec_args
{
    int argc;
    char *argv[EXEC_MAX_ARGS];
};

int exec_args_copy_from_user(
    struct exec_args *args,
    struct page_directory *directory,
    const char *const *user_argv
);

void exec_args_destroy(struct exec_args *args);
