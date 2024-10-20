/*
 * Copyright (c) 2016 Open-RnD Sp. z o.o.
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/printk.h>
#include <inttypes.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(main);

#include "buttons.h"

/*
 * The led0 devicetree alias is optional. If present, we'll use it
 * to turn on the LED whenever the button is pressed.
 */
static struct gpio_dt_spec led = GPIO_DT_SPEC_GET_OR(DT_ALIAS(led0), gpios, {0});

void button_pressed(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
    LOG_INF("Button pressed at %" PRIu32 "", k_cycle_get_32());
}

int main(void)
{
    int ret;

    if (!gpio_is_ready_dt(&led)) {
        LOG_ERR("LED device %s is not ready; ignoring it", led.port->name);
        return -ENODEV;
    }

    ret = gpio_pin_configure_dt(&led, GPIO_OUTPUT);
    if (ret != 0) {
        LOG_ERR("Error %d: failed to configure LED device %s pin %d", ret, led.port->name, led.pin);
        return ret;
    }

    ret = buttons_init();
    if (ret != 0) {
        LOG_ERR("Error %d: failed to init buttons", ret);
        return ret;
    }

    while (true) {
        gpio_pin_set_dt(&led, button_is_pressed(0));
        k_sleep(K_MSEC(10));
    }

    return 0;
}
