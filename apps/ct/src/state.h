#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef struct STATE_s
{
    int16_t temperature;
    bool probe_connected;
    uint8_t battery_percentage;
} STATE_t;

extern STATE_t state;

int state_init(void);
