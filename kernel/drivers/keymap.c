#include "keymap.h"

static keymap_t current_layout = 0;

void keymap_set(keymap_t layout)
{
    current_layout = layout;
}

char keymap_translate(uint8_t scancode)
{
    if (current_layout == 0)
        return 0;

    return current_layout(scancode);
}
