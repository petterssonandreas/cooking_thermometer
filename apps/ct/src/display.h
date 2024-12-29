#pragma once

#include <stdint.h>

int display_init(void);

void display_draw_time(void);

void display_draw_voltage(int16_t mvolt, int16_t raw);

void display_draw_temperature(int16_t temperature);
