#pragma once

#include <stdbool.h>
#include <stdint.h>

bool kb_io_init(void);
// Active-low keypad lines are returned as set bits when pressed.
bool kb_io_read_buttons(uint16_t *pressed_mask);
