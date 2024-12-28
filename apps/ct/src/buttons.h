#pragma once

#include <stdbool.h>
#include <stddef.h>

bool button_is_pressed(size_t button_index);

int buttons_init(void);
