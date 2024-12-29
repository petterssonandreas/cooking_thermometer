#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef enum
{
    STATE_INIT,
    STATE_IDLE,
    STATE_SET_TARGET_TEMP,
} STATE_e;


typedef struct STATE_s
{
    STATE_e state;

    int16_t temperature;
    int16_t target_temperature;
    bool probe_connected;
    uint8_t battery_percentage;

    bool blink;
} STATE_t;

extern STATE_t state;

typedef enum
{
    STATE_EVENT_BUTTON_PRESSED,
    STATE_EVENT_BUTTON_RELEASED,
    STATE_EVENT_SET_TARGET_TEMP_TIMER_ELAPSED,
} STATE_Event_e;

typedef struct state_queue_entry_s {
    STATE_Event_e event;
    uint32_t data;
} state_queue_entry_t;

extern struct k_msgq state_msgq;

int state_init(void);

void state_reset_blink_timer(void);
