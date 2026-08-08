#include "vga.h"
#include "io.h"

#define FRAMEBUFFER ((volatile uint16_t *)0xB8000)

static uint16_t cursor_row = 0;
static uint16_t cursor_col = 0;

static uint8_t foreground = LIGHT_GRAY;
static uint8_t background = BLACK;

void vga_write_cell(uint16_t row,
                uint16_t col,
                char c,
                uint8_t fg,
                uint8_t bg)
{
    uint16_t index = row * VGA_WIDTH + col;
    uint16_t attribute = ((uint16_t)bg << 12) | ((uint16_t)fg << 8);

    FRAMEBUFFER[index] = attribute | (uint8_t)c;
}


void vga_clear(uint8_t fg, uint8_t bg)
{
    foreground = fg;
    background = bg;

    for (uint16_t row = 0; row < VGA_HEIGHT; row++)
    {
        for (uint16_t col = 0; col < VGA_WIDTH; col++)
        {
            vga_write_cell(row, col, ' ', fg, bg);
        }
    }

    cursor_row = 0;
    cursor_col = 0;

    vga_update_cursor();
}

void vga_putchar(char c)
{
    switch (c)
    {
        case '\n':
            cursor_row++;
            cursor_col = 0;
            break;

        case '\r':
            cursor_col = 0;
            break;

        default:
            vga_write_cell(cursor_row,
                       cursor_col,
                       c,
                       foreground,
                           background);

                       cursor_col++;

                       if (cursor_col >= VGA_WIDTH)
                       {
                           cursor_col = 0;
                           cursor_row++;
                       }
                       break;
    }

    if (cursor_row >= VGA_HEIGHT)
    {
        vga_scroll();
        cursor_row = VGA_HEIGHT - 1;
    }
    vga_update_cursor();
}

void vga_scroll(void)
{
    /* Move every row up one line */
    for (uint16_t row = 1; row < VGA_HEIGHT; row++)
    {
        for (uint16_t col = 0; col < VGA_WIDTH; col++)
        {
            FRAMEBUFFER[(row - 1) * VGA_WIDTH + col] =
            FRAMEBUFFER[row * VGA_WIDTH + col];
        }
    }

    /* Clear the last row */
    for (uint16_t col = 0; col < VGA_WIDTH; col++)
    {
        vga_write_cell(VGA_HEIGHT - 1,
                   col,
                   ' ',
                   foreground,
                       background);
    }
}

void vga_write(const char *str)
{
    while (*str)
    {
        vga_putchar(*str++);
    }
}


void vga_update_cursor(void)
{
    uint16_t position = cursor_row * VGA_WIDTH + cursor_col;

    outb(0x3D4, 0x0F);
    outb(0x3D5, (uint8_t)(position & 0xFF));

    outb(0x3D4, 0x0E);
    outb(0x3D5, (uint8_t)(position >> 8));
}

void vga_set_cursor(uint16_t row, uint16_t col)
{
    cursor_row = row;
    cursor_col = col;
    vga_update_cursor();
}
