#include "debug.h"

#include "printf.h"
#include "kernel.h"
#include "types.h"

static void print_addr(uint32_t value)
{
    static const char digits[] = "0123456789ABCDEF";
    char buffer[11];
    int i;

    buffer[0] = '0';
    buffer[1] = 'x';

    for (i = 0; i < 8; i++)
        buffer[2 + i] = digits[(value >> (28 - 4 * i)) & 0xF];

    buffer[10] = '\0';

    printf("%s", buffer);
}

void debug_backtrace(int skip, int max_frames)
{
    uint32_t *ebp;
    int frame;

    __asm__ volatile ("mov %%ebp, %0" : "=r" (ebp));

    while (skip-- > 0 && ebp != NULL && ((uintptr_t)ebp & 0x3) == 0)
        ebp = (uint32_t *)ebp[0];

    if (max_frames <= 0 || max_frames > DEBUG_BACKTRACE_MAX_FRAMES)
        max_frames = DEBUG_BACKTRACE_MAX_FRAMES;

    printf("Backtrace:\n");

    for (frame = 0; frame < max_frames; frame++)
    {
        uint32_t return_addr;
        uint32_t *next_ebp;

        if (ebp == NULL || ((uintptr_t)ebp & 0x3) != 0)
        {
            printf("  #%d <broken frame pointer, stopping>\n", frame);
            break;
        }

        return_addr = ebp[1];
        next_ebp    = (uint32_t *)ebp[0];

        printf("  #%d ", frame);
        print_addr(return_addr);

        if (return_addr < 0x00100000 || return_addr >= (uintptr_t)&kernel_end)
        {
            printf(" <outside kernel image, stopping>\n");
            break;
        }

        printf("\n");

        if (return_addr == 0)
            break;

        if (next_ebp <= ebp)
            break;

        ebp = next_ebp;
    }
}
