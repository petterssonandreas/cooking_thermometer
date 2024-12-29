/*
 * Copyright (c) 2016 Open-RnD Sp. z o.o.
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "alarm.h"
#include "battery.h"
#include "state.h"
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/printk.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(ct_main, CONFIG_CT_MAIN_LOG_LEVEL);

#include "adc.h"
#include "buttons.h"
#include "display.h"
#include "cttime.h"

static void set_target_temp_timer_expiry_function(struct k_timer *timer_id) {
    state_queue_entry_t queue_entry;
    queue_entry.event = STATE_EVENT_SET_TARGET_TEMP_TIMER_ELAPSED;
    if (k_msgq_put(&state_msgq, &queue_entry, K_NO_WAIT)) {
        LOG_WRN("Failed to put target temp timer event in msgq");
    }
}

K_TIMER_DEFINE(set_target_temp_timer, set_target_temp_timer_expiry_function, NULL);

static void check_inputs(void)
{
    state_queue_entry_t queue_entry;

    switch (state.state) {
    case STATE_IDLE:
        if (state.probe_connected && (state.temperature >= state.target_temperature)) {
            queue_entry.event = STATE_EVENT_TARGET_TEMP_REACHED;
            if (k_msgq_put(&state_msgq, &queue_entry, K_NO_WAIT)) {
                LOG_WRN("Failed to put target temp reached event in msgq");
            }
        }
        break;

    case STATE_TARGET_TEMP_REACHED:
    case STATE_TARGET_TEMP_REACHED_ACKED:
        if (!state.probe_connected || (state.temperature < state.target_temperature)) {
            queue_entry.event = STATE_EVENT_TARGET_TEMP_LOST;
            if (k_msgq_put(&state_msgq, &queue_entry, K_NO_WAIT)) {
                LOG_WRN("Failed to put target temp lost event in msgq");
            }
        }
        break;

    default:
        break;
    }
}

static void state_loop(void)
{
    int ret;
    state_queue_entry_t queue_entry;

    ret = k_msgq_get(&state_msgq, &queue_entry, K_MSEC(20));
    if (ret) {
        return;
    }

    switch (state.state) {
    case STATE_IDLE:
        switch (queue_entry.event) {
        case STATE_EVENT_BUTTON_PRESSED:
            if (queue_entry.data == 2) {
                state.state = STATE_SET_TARGET_TEMP;
                state_reset_blink_timer();
                k_timer_start(&set_target_temp_timer, K_SECONDS(5), K_NO_WAIT);
            }
            break;

        case STATE_EVENT_TARGET_TEMP_REACHED:
            state.state = STATE_TARGET_TEMP_REACHED;
            alarm_enable();
            break;

        default:
            break;
        }
        break;

    case STATE_TARGET_TEMP_REACHED:
        switch (queue_entry.event) {
        case STATE_EVENT_BUTTON_PRESSED:
            /* Allow any button to ack */
            state.state = STATE_TARGET_TEMP_REACHED_ACKED;
            alarm_disable();
            break;

        case STATE_EVENT_TARGET_TEMP_LOST:
            state.state = STATE_IDLE;
            alarm_disable();
            break;

        default:
            break;
        }
        break;

    case STATE_TARGET_TEMP_REACHED_ACKED:
        switch (queue_entry.event) {
        case STATE_EVENT_TARGET_TEMP_LOST:
            state.state = STATE_IDLE;
            alarm_disable();
            break;

        case STATE_EVENT_BUTTON_PRESSED:
            if (queue_entry.data == 2) {
                state.state = STATE_SET_TARGET_TEMP;
                state_reset_blink_timer();
                k_timer_start(&set_target_temp_timer, K_SECONDS(5), K_NO_WAIT);
            }
            break;

        default:
            break;
        }
        break;

    case STATE_SET_TARGET_TEMP:
        switch (queue_entry.event) {
        case STATE_EVENT_BUTTON_PRESSED:
            if (queue_entry.data == 0) {
                state.target_temperature++;
                state_reset_blink_timer();
                k_timer_start(&set_target_temp_timer, K_SECONDS(5), K_NO_WAIT);
            }
            else if (queue_entry.data == 1) {
                state.target_temperature--;
                state_reset_blink_timer();
                k_timer_start(&set_target_temp_timer, K_SECONDS(5), K_NO_WAIT);
            }
            else if (queue_entry.data == 2) {
                state.state = STATE_IDLE;
                k_timer_stop(&set_target_temp_timer);
            }
            break;

        case STATE_EVENT_SET_TARGET_TEMP_TIMER_ELAPSED:
            state.state = STATE_IDLE;
            break;

        default:
            break;
        }
        break;

    default:
        __ASSERT(false, "Bad state: %d", state.state);
        break;
    }
}

int main(void)
{
    int ret;

    LOG_INF("main starting");

    ret = state_init();
    if (ret) {
        LOG_ERR("Error %d: failed to init state", ret);
        return ret;
    }
    LOG_INF("state init done");

    ret = alarm_init();
    if (ret) {
        LOG_ERR("Error %d: failed to init alarm", ret);
        return ret;
    }
    LOG_INF("alarm init done");

    ret = buttons_init();
    if (ret != 0) {
        LOG_ERR("Error %d: failed to init buttons", ret);
        return ret;
    }
    LOG_INF("buttons init done");

    ret = cttime_init();
    if (ret) {
        LOG_ERR("Error %d: failed to init time", ret);
        return ret;
    }
    LOG_INF("Time init done");

    ret = adc_init();
    if (ret) {
        LOG_ERR("Error %d: failed to init ADC", ret);
        return ret;
    }
    LOG_INF("ADC init done");

    ret = battery_init();
    if (ret) {
        LOG_ERR("Error %d: failed to init battery", ret);
        return ret;
    }
    LOG_INF("battery init done");

    ret = display_init();
    if (ret) {
        LOG_ERR("Error %d: failed to init display", ret);
        return ret;
    }
    LOG_INF("Display init done");

    state.state = STATE_IDLE;

    while (true) {
        state_loop();
        check_inputs();
    }

    return 0;
}
