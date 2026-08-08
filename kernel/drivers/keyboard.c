#include "keyboard.h"
#include "keymap.h"
#include "io.h"
#include "console.h"

void keyboard_handler(void)
{
    uint8_t scancode = inb(0x60);

    /* Ignore key releases */
    if (scancode & 0x80)
        return;

    char c = keymap_translate(scancode);

    if (c)
        console_putchar(c);
}
void keyboard_init(keymap_t layout)
{
    keymap_set(layout);
}
