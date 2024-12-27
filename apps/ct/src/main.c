/*
 * Copyright (c) 2016 Open-RnD Sp. z o.o.
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "zephyr/display/cfb.h"
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/display.h>
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

#define FONT_IDX 1

static int display_init(void)
{
    const struct device *dev;
    uint16_t x_res;
    uint16_t y_res;
    uint16_t rows;
    uint8_t ppt;
    uint8_t font_width;
    uint8_t font_height;

    dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));
    if (!device_is_ready(dev)) {
        LOG_ERR("Device %s not ready", dev->name);
        return 0;
    }

    if (display_set_pixel_format(dev, PIXEL_FORMAT_MONO10) != 0) {
        if (display_set_pixel_format(dev, PIXEL_FORMAT_MONO01) != 0) {
            LOG_ERR("Failed to set required pixel format");
            return 0;
        }
    }

    LOG_INF("Initialized %s", dev->name);

    if (cfb_framebuffer_init(dev)) {
        LOG_ERR("Framebuffer initialization failed!");
        return 0;
    }

    cfb_framebuffer_clear(dev, true);

    display_blanking_off(dev);

    x_res = cfb_get_display_parameter(dev, CFB_DISPLAY_WIDTH);
    y_res = cfb_get_display_parameter(dev, CFB_DISPLAY_HEIGH);
    rows = cfb_get_display_parameter(dev, CFB_DISPLAY_ROWS);
    ppt = cfb_get_display_parameter(dev, CFB_DISPLAY_PPT);

    if (cfb_get_font_size(dev, FONT_IDX, &font_width, &font_height))
    {
        LOG_ERR("Failed to get font for index %u", FONT_IDX);
        return -EBFONT;
    }
    cfb_framebuffer_set_font(dev, FONT_IDX);
    LOG_INF("font width %d, font height %d", font_width, font_height);

    LOG_INF("x_res %d, y_res %d, ppt %d, rows %d, cols %d", x_res, y_res, ppt, rows, cfb_get_display_parameter(dev, CFB_DISPLAY_COLS));

    // cfb_framebuffer_invert(dev);

    cfb_set_kerning(dev, 0);

    cfb_framebuffer_clear(dev, false);
    cfb_print(dev, "Booting", 0, 24);
    cfb_framebuffer_finalize(dev);

    return 0;
}



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

    ret = display_init();
    if (ret) {
        LOG_ERR("Error %d: failed to init display", ret);
        return ret;
    }

    LOG_INF("Display init done");

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
