#pragma once

#include "types.h"

void console_init(void);
void console_putchar(char c);
void console_write(const char *str);
void console_clear(void);
void console_set_color(uint8_t fg, uint8_t bg);
