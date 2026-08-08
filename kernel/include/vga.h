#pragma once

#include "types.h"
#include "colors.h"

/* Screen dimensions */
#define VGA_WIDTH  80
#define VGA_HEIGHT 25

/* Screen management */
void vga_clear(uint8_t fg, uint8_t bg);

/* Cursor position */
void vga_set_cursor(uint16_t row, uint16_t col);
void vga_get_cursor(uint16_t *row, uint16_t *col);
void vga_set_cursor(uint16_t row, uint16_t col);
void vga_update_cursor(void);

/* Character output */
void vga_putchar(char c);
void vga_write(const char *str);
void vga_write_at(const char *str,
              uint16_t row,
              uint16_t col);

/* Colored output */
void vga_putchar_color(char c,
                   uint8_t fg,
                   uint8_t bg);

void vga_write_color(const char *str,
                 uint8_t fg,
                 uint8_t bg);

void vga_write_at_color(const char *str,
                    uint16_t row,
                    uint16_t col,
                    uint8_t fg,
                    uint8_t bg);

/* Low-level cell access */
void vga_write_cell(uint16_t row,
                uint16_t col,
                char c,
                uint8_t fg,
                uint8_t bg);

/* Cursor control */
void vga_enable_cursor(void);
void vga_disable_cursor(void);

/* Scrolling */
void vga_scroll(void);
