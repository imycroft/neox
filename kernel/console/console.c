#include "console.h"
#include "vga.h"

void console_init(void)
{
    vga_clear(LIGHT_GRAY, BLACK);
}

void console_putchar(char c)
{
    vga_putchar(c);
}

void console_write(const char *str)
{
    vga_write(str);
}

void console_clear(void)
{
    vga_clear(LIGHT_GRAY, BLACK);
}
