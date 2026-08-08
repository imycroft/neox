#pragma once

#include "types.h"

#define DEFAULT_KEYMAP keymap_azerty

typedef char (*keymap_t)(uint8_t scancode);

/* Select the active keyboard layout */
void keymap_set(keymap_t layout);

/* Translate using the active layout */
char keymap_translate(uint8_t scancode);

/* Available layouts */
char keymap_qwerty(uint8_t scancode);
char keymap_azerty(uint8_t scancode);
