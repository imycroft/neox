#include "keymap.h"

static const char table[128] =
{
    /* table later */
};

char keymap_qwerty(uint8_t scancode)
{
    if (scancode >= 128)
        return 0;

    return table[scancode];
}
