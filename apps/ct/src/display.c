#include "display.h"

#include <zephyr/display/cfb.h>
#include <zephyr/drivers/display.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(ct_display, CONFIG_CT_DISPLAY_LOG_LEVEL);

#define FONT_IDX 1

int display_init(void)
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
