/*
 * Copyright (c) 2016 Open-RnD Sp. z o.o.
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

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
#include "probe.h"

#define PWM_LED_PERIOD NSEC_PER_MSEC

static const struct gpio_dt_spec buzzer = GPIO_DT_SPEC_GET(DT_ALIAS(buzzer), gpios);
static const struct pwm_dt_spec ui_led = PWM_DT_SPEC_GET(DT_ALIAS(uiled));

static void set_target_temp_timer_expiry_function(struct k_timer *timer_id) {
    state_queue_entry_t queue_entry;
    queue_entry.event = STATE_EVENT_SET_TARGET_TEMP_TIMER_ELAPSED;
    if (k_msgq_put(&state_msgq, &queue_entry, K_NO_WAIT)) {
        LOG_WRN("Failed to put target temp timer event in msgq");
    }
}

K_TIMER_DEFINE(set_target_temp_timer, set_target_temp_timer_expiry_function, NULL);

int main(void)
{
    int ret;
    state_queue_entry_t queue_entry;

    LOG_INF("main starting");

    ret = state_init();
    if (ret) {
        LOG_ERR("Error %d: failed to init state", ret);
        return ret;
    }
    LOG_INF("state init done");

    if (!gpio_is_ready_dt(&buzzer)) {
        LOG_ERR("Error: buzzer GPIO is not ready");
        return -ENODEV;
    }
    ret = gpio_pin_configure_dt(&buzzer, GPIO_OUTPUT_INACTIVE);
    if (ret < 0) {
        LOG_ERR("Error %d: failed to configure buzzer gpio", ret);
        return ret;
    }
    LOG_INF("Buzzer init done");

    if (!pwm_is_ready_dt(&ui_led)) {
        LOG_ERR("Error: PWM device %s is not ready", ui_led.dev->name);
        return -ENODEV;
    }
    ret = pwm_set_dt(&ui_led, PWM_LED_PERIOD, 0);
    if (ret < 0) {
        LOG_ERR("Error %d: failed to set pulse width", ret);
        return ret;
    }
    LOG_INF("PWM LEDs init done");

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

    ret = probe_init();
    if (ret) {
        LOG_ERR("Error %d: failed to init probe", ret);
        return ret;
    }
    LOG_INF("probe init done");

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
        if (k_msgq_get(&state_msgq, &queue_entry, K_FOREVER)) {
            LOG_WRN("Failed to get message from message queue");
            continue;
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

    return 0;
}
