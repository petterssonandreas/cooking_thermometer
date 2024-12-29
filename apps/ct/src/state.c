#include "state.h"

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(ct_state, CONFIG_CT_STATE_LOG_LEVEL);

/* The state variable, global */
STATE_t state;

K_MSGQ_DEFINE(state_msgq, sizeof(state_queue_entry_t), 20, 1);

/* Timer object */
static struct k_timer blink_timer;

/* Timer callback function called on expiration */
static void blink_timer_expiry_function(struct k_timer *timer_id) {
    state.blink = !state.blink;
}

int state_init(void)
{
    memset(&state, 0, sizeof(state));

    state.state = STATE_INIT;
    state.temperature = INT16_MIN;
    state.target_temperature = 25;
    state.probe_connected = false;
    state.battery_percentage = 0;
    state.blink = true;

    k_timer_init(&blink_timer, blink_timer_expiry_function, NULL);
    k_timer_start(&blink_timer, K_MSEC(500), K_MSEC(500));

    return 0;
}

void state_reset_blink_timer(void)
{
    state.blink = true;
    k_timer_start(&blink_timer, K_MSEC(500), K_MSEC(500));
}
