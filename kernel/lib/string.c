#include "string.h"

size_t strlen(const char *str)
{
    size_t len = 0;

    while (*str++)
        len++;

    return len;
}

void *memset(void *dest, int value, size_t n)
{
    uint8_t *ptr = (uint8_t *)dest;

    while (n--)
        *ptr++ = (uint8_t)value;

    return dest;
}

void *memcpy(void *dest, const void *src, size_t n)
{
    uint8_t *d = (uint8_t *)dest;
    const uint8_t *s = (const uint8_t *)src;

    while (n--)
        *d++ = *s++;

    return dest;
}

int memcmp(const void *a, const void *b, size_t n)
{
    const uint8_t *p1 = (const uint8_t *)a;
    const uint8_t *p2 = (const uint8_t *)b;

    while (n--)
    {
        if (*p1 != *p2)
            return *p1 - *p2;

        p1++;
        p2++;
    }

    return 0;
}

char *strcpy(char *dest, const char *src)
{
    char *ret = dest;

    while ((*dest++ = *src++) != '\0')
        ;

    return ret;
}

char *strncpy(char *dest,
              const char *src,
              size_t n)
{
    char *ret = dest;

    while (n && *src)
    {
        *dest++ = *src++;
        n--;
    }

    while (n)
    {
        *dest++ = '\0';
        n--;
    }

    return ret;
}

int strcmp(const char *a, const char *b)
{
    while (*a && (*a == *b))
    {
        a++;
        b++;
    }

    return (unsigned char)*a - (unsigned char)*b;
}
