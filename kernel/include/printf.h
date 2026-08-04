#pragma once

#include "stdarg.h"

int printf(const char *fmt, ...);
int vprintf(const char *fmt, va_list args);

//static void print_hex(uint32_t value);
