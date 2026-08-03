#include "assert.h"

#include "printf.h"

void assert_fail(const char *expr,
                 const char *file,
                 int line)
{
    printf("\n");

    printf("========================================\n");
    printf("        KERNEL ASSERTION FAILED\n");
    printf("========================================\n");

    printf("Expression : %s\n", expr);
    printf("File       : %s\n", file);
    printf("Line       : %d\n", line);

    printf("\nSystem halted.\n");

    for (;;)
    {
        __asm__ volatile ("cli");
        __asm__ volatile ("hlt");
    }
}
