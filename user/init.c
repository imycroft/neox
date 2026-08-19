#include "unistd.h"

void _start(void)
{
    static const char message[] =
    "[NEOX] Hello from init!\n";

    write(
        1,
        message,
        sizeof(message) - 1
    );

    exit(0);
}
