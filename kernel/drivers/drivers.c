#include "drivers.h"

#include "console.h"
#include "keyboard.h"

void drivers_init(void)
{
    keyboard_init(DEFAULT_KEYMAP);
}
