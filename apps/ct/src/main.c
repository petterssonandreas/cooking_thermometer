/*
 * Copyright (c) 2016 Open-RnD Sp. z o.o.
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/printk.h>

// #include <drivers/active_buzzer.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(main);

#include "buttons.h"

#define PWM_LED_PERIOD NSEC_PER_MSEC

static const struct gpio_dt_spec buzzer = GPIO_DT_SPEC_GET(DT_ALIAS(buzzer), gpios);
static const struct pwm_dt_spec ui_led = PWM_DT_SPEC_GET(DT_ALIAS(uiled));


int main(void)
{
    int ret;

    LOG_INF("main starting");

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

    while (true) {
        if (button_is_pressed(0)) {
            pwm_set_pulse_dt(&ui_led, PWM_LED_PERIOD);
            gpio_pin_set_dt(&buzzer, 1);
        }
        else {
            pwm_set_pulse_dt(&ui_led, 0);
            gpio_pin_set_dt(&buzzer, 0);
        }
        k_sleep(K_MSEC(10));
    }

    return 0;
}
