#include "display.h"

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/display/cfb.h>
#include <zephyr/drivers/display.h>
#include <stdio.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(ct_display, CONFIG_CT_DISPLAY_LOG_LEVEL);

#include "cttime.h"

#define FONT_IDX 1

static const struct device *dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));

int display_init(void)
{
    uint16_t x_res;
    uint16_t y_res;
    uint16_t rows;
    uint8_t ppt;
    uint8_t font_width;
    uint8_t font_height;

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

void display_draw_time(void)
{
    int ret;
    struct rtc_time tm;

    ret = cttime_get_date_time(&tm);
    if (ret) {
        LOG_ERR("Failed to get time: %d", ret);
        return;
    }

    // LOG_INF("RTC date and time: %04d-%02d-%02d %02d:%02d:%02d", tm.tm_year + 1900,
    //     tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);

    char s[16];
    snprintf(s, sizeof(s), "%02d:%02d:%02d", tm.tm_hour, tm.tm_min, tm.tm_sec);

    cfb_framebuffer_clear(dev, false);
    cfb_print(dev, s, 0, 24);
    cfb_framebuffer_finalize(dev);
}