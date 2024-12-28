#include "cttime.h"

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/rtc.h>
#include <zephyr/sys/util.h>


#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(ct_time, CONFIG_CT_TIME_LOG_LEVEL);

static const struct device *const rtc = DEVICE_DT_GET(DT_ALIAS(rtc));

static int set_initial_date_time(void)
{
    int ret = 0;
    struct rtc_time tm = {
        .tm_year = 2024 - 1900,
        .tm_mon = 12 - 1,
        .tm_mday = 1,
        .tm_hour = 12,
        .tm_min = 0,
        .tm_sec = 0,
    };

    ret = rtc_set_time(rtc, &tm);
    if (ret < 0) {
        LOG_ERR("Cannot write date time: %d", ret);
        return ret;
    }
    return ret;
}

int cttime_get_date_time(struct rtc_time *tm)
{
    int ret = 0;

    ret = rtc_get_time(rtc, tm);
    if (ret < 0) {
        LOG_ERR("Cannot read date time: %d", ret);
        return ret;
    }

    return ret;
}

int cttime_init(void)
{
    if (!device_is_ready(rtc)) {
        LOG_ERR("Device is not ready");
        return -ENODEV;
    }

    return set_initial_date_time();
}
