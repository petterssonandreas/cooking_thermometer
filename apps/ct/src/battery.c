#include "battery.h"
#include "adc.h"
#include "state.h"
#include <stdlib.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/shell/shell.h>
#include <zephyr/sys/util.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(ct_battery, CONFIG_CT_BATTERY_LOG_LEVEL);

typedef struct CurveValue_s
{
    uint16_t voltage_mv;
    uint8_t percentage;
} CurveValue_t;

static CurveValue_t BatteryCurve[] =
{
    {.voltage_mv=2500, .percentage=0},
    {.voltage_mv=3450, .percentage=20},
    {.voltage_mv=3600, .percentage=40},
    {.voltage_mv=3700, .percentage=60},
    {.voltage_mv=3850, .percentage=80},
    {.voltage_mv=4200, .percentage=100},
};

static uint8_t interpolate_battery_level(uint16_t voltage_mv)
{
    if (voltage_mv < BatteryCurve[0].voltage_mv) {
        return 0;
    }
    if (voltage_mv > BatteryCurve[ARRAY_SIZE(BatteryCurve) - 1].voltage_mv) {
        return 100;
    }

    for (size_t i = 0; i < ARRAY_SIZE(BatteryCurve) - 1; i++)
    {
        if ((voltage_mv >= BatteryCurve[i].voltage_mv) && (voltage_mv < BatteryCurve[i + 1].voltage_mv)) {
            int32_t voltage_range = abs(BatteryCurve[i + 1].voltage_mv - BatteryCurve[i].voltage_mv);
            int32_t percentage_range = abs(BatteryCurve[i + 1].percentage - BatteryCurve[i].percentage);
            int32_t offset_into_voltage = abs(voltage_mv - BatteryCurve[i].voltage_mv);
            return ((offset_into_voltage * percentage_range) / voltage_range) + BatteryCurve[i].percentage;
        }
    }

    /* Should not get here */
    return 0;
}

static K_THREAD_STACK_DEFINE(battery_thread_stack, 1024);

static struct k_thread battery_thread;

static void battery_fn(void *p1, void *p2, void *p3)
{
    UNUSED(p1);
    UNUSED(p2);
    UNUSED(p3);

    int ret;
    int16_t vbat_mv;

    LOG_INF("Battery thread started");
    while (true) {
        ret = adc_get_battery_voltage(&vbat_mv);
        if (ret) {
            LOG_ERR("Failed to get battery voltage");
        }
        else {
            state.battery_percentage = interpolate_battery_level(vbat_mv);
            LOG_DBG("Battery level: %u %%", state.battery_percentage);
        }

        /* No need to check this very often */
        k_sleep(K_SECONDS(10));
    }

    return;
}

int battery_init(void)
{
    k_thread_create(
        &battery_thread,
        battery_thread_stack,
        K_THREAD_STACK_SIZEOF(battery_thread_stack),
        battery_fn,
        NULL, NULL, NULL,
        K_LOWEST_APPLICATION_THREAD_PRIO,
        0,
        K_NO_WAIT);

    return 0;
}

static int battery_cmd(const struct shell *sh, size_t argc, char **argv)
{
    UNUSED(argc);
    UNUSED(argv);

    int ret;
    int16_t vbat_mv;
    uint8_t battery_percentage;

    ret = adc_get_battery_voltage(&vbat_mv);
    if (ret) {
        shell_print(sh, "Failed to get battery voltage");
        return ret;
    }

    battery_percentage = interpolate_battery_level(vbat_mv);

    shell_print(sh, "Battery info:");
    shell_print(sh, "   Voltage:    %d mV", vbat_mv);
    shell_print(sh, "   Percentage: %u%%", battery_percentage);
    return 0;
}

SHELL_CMD_REGISTER(battery, NULL, "Battery info", battery_cmd);
