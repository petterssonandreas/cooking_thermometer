#pragma once

#include <zephyr/drivers/rtc.h>
#include <zephyr/sys/util.h>

int cttime_init(void);

int cttime_get_date_time(struct rtc_time *tm);
