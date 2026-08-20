#include "unistd.h"

void _start(void)
{
    static const char message[] =
    "[NEOX] Hello from exec yoooo!\n";

    ssize_t count =  write(
        1,
        message,
        sizeof(message) - 1
    );

    if (count < 0)
    {
        exit(1);
    }

    exit(0);
}
