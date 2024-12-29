#include "state.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(ct_state, CONFIG_CT_STATE_LOG_LEVEL);

/* The state variable, global */
STATE_t state;

int state_init(void)
{
    memset(&state, 0, sizeof(state));

    state.temperature = INT16_MIN;
    state.probe_connected = false;
    state.battery_percentage = 0;
    return 0;
}
