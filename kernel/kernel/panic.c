#include "panic.h"
#include "console.h"

void panic(const char *message)
{
    console_write("\n\n*** KERNEL PANIC ***\n");
    console_write(message);
    console_write("\n");

    __asm__ volatile ("cli");

    for (;;)
        __asm__ volatile ("hlt");
}
