#include "assert.h"
#include "debug.h"
#include "types.h"
#include "printf.h"


void assert_fail(const char *expr,
                 const char *file,
                 int line,
                 void *caller
                 )
{
    printf("\n");

    printf("========================================\n");
    printf("        KERNEL ASSERTION FAILED\n");
    printf("========================================\n");

    printf("Expression : %s\n", expr);
    printf("File       : %s\n", file);
    printf("Line       : %d\n", line);
    printf("Caller     : 0x%x\n", (uintptr_t)caller);

    printf("\n");
    debug_backtrace(1, DEBUG_BACKTRACE_MAX_FRAMES);   /* skip=1 hides assert_fail()'s own frame */

    printf("\nSystem halted.\n");

    for (;;)
    {
        __asm__ volatile ("cli");
        __asm__ volatile ("hlt");
    }
}
