#pragma once

#include "types.h"

size_t strlen(const char *str);

void *memcpy(void *dest, const void *src, size_t n);
void *memmove(void *dest, const void *src, size_t n);
void *memset(void *dest, int value, size_t n);
int memcmp(const void *a, const void *b, size_t n);

char *strcpy(char *dest, const char *src);
char *strncpy(char *dest, const char *src, size_t n);

int strcmp(const char *a, const char *b);
int strncmp(const char *a, const char *b, size_t n);
