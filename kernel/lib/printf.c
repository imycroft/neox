#include "printf.h"
#include "console.h"
#include "types.h"

static int print_unsigned(unsigned int value, uint32_t base)
{
    char buffer[32];
    int written = 0;
    uint32_t i = 0;

    if (value == 0)
    {
        console_putchar('0');
        return 1;
    }

    while (value)
    {
        uint32_t digit = value % base;

        if (digit < 10)
            buffer[i++] = '0' + digit;
        else
            buffer[i++] = 'A' + (digit - 10);

        value /= base;
    }

    written = (int)i;

    while (i)
        console_putchar(buffer[--i]);

    return written;
}

static int print_signed(int value)
{

    if (value < 0)
    {
        console_putchar('-');
        return 1 + print_unsigned((uint32_t)(-(value + 1)) + 1, 10);
    }
    return print_unsigned((uint32_t)value, 10);
}

int vprintf(const char *fmt, va_list args)
{
    int written = 0;

    while (*fmt)
    {
        if (*fmt != '%')
        {
            console_putchar(*fmt++);
            written++;
            continue;
        }

        fmt++;

        switch (*fmt)
        {
            case '%':
                console_putchar('%');
                written++;
                break;

            case 'c':
            {
                char c = (char)va_arg(args, int);
                console_putchar(c);
                written++;
                break;
            }

            case 's':
            {
                const char *str = va_arg(args, const char *);

                if (str == NULL)
                    str = "(null)";

                while (*str)
                {
                    console_putchar(*str++);
                    written++;
                }

                break;
            }

            case 'd':
            {
                int value = va_arg(args, int);
                written += print_signed(value);
                break;
            }

            case 'u':
            {
                unsigned int value = va_arg(args, unsigned int);
                written += print_unsigned(value, 10);
                break;
            }

            case 'x':
            {
                uint32_t value = va_arg(args, uint32_t);
                written += print_unsigned(value, 16);
                break;
            }

            default:
                console_putchar('%');
                console_putchar(*fmt);
                written += 2;
                break;
        }

        fmt++;
    }

    return written;
}

int printf(const char *fmt, ...)
{
    va_list args;

    va_start(args, fmt);
    int written = vprintf(fmt, args);
    va_end(args);

    return written;
}
