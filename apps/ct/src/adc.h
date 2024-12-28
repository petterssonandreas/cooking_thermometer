#pragma once

#include <stdint.h>

int adc_init(void);

int adc_get_battery_voltage(int16_t *vbat_mv);

int adc_get_temperature_raw(int16_t *temperature_raw);
