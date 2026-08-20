#include "exec.h"

#include "heap.h"
#include "paging.h"
#include "string.h"

int exec_args_copy_from_user(
    struct exec_args *args,
    struct page_directory *directory,
    const char *const *user_argv
)
{
    uint32_t i;

    if (args == NULL)
        return -1;

    if (directory == NULL)
        return -1;

    if (user_argv == NULL)
        return -1;

    memset(args, 0, sizeof(*args));

    for (i = 0; i < EXEC_MAX_ARGS; i++)
    {
        uintptr_t pointer_address;
        const char *user_string;
        size_t length;
        char *copy;

        pointer_address =
        (uintptr_t)&user_argv[i];

        if (!paging_user_range_valid(
            directory,
            pointer_address,
            sizeof(uintptr_t)))
        {
            goto fail;
        }

        user_string = user_argv[i];

        if (user_string == NULL)
        {
            args->argc = (int)i;
            return 0;
        }

        if (!paging_user_string_valid(
            directory,
            user_string))
        {
            goto fail;
        }

        length = 0;

        while (user_string[length] != '\0')
        {
            length++;

            if (length >= EXEC_MAX_ARG_LENGTH)
                goto fail;
        }

        copy = kmalloc(length + 1);

        if (copy == NULL)
            goto fail;

        memcpy(
            copy,
            user_string,
            length + 1
        );

        args->argv[i] = copy;
    }

    /*
     * No NULL terminator was found within EXEC_MAX_ARGS.
     */
    goto fail;

    fail:

    exec_args_destroy(args);

    return -1;
}

void exec_args_destroy(struct exec_args *args)
{
    uint32_t i;

    if (args == NULL)
        return;

    for (i = 0; i < EXEC_MAX_ARGS; i++)
    {
        if (args->argv[i] != NULL)
        {
            kfree(args->argv[i]);
            args->argv[i] = NULL;
        }
    }

    args->argc = 0;
}
