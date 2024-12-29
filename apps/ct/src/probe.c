#include "state.h"
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/util.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(ct_probe, CONFIG_CT_PROBE_LOG_LEVEL);

static const struct gpio_dt_spec jack = GPIO_DT_SPEC_GET_OR(DT_ALIAS(jack_connected), gpios, {0});

static K_THREAD_STACK_DEFINE(probe_thread_stack, 1024);

static struct k_thread probe_thread;

static void probe_fn(void *p1, void *p2, void *p3)
{
    UNUSED(p1);
    UNUSED(p2);
    UNUSED(p3);

    int prev_jack_value = 0;
    int jack_value = 0;

    while (true) {
        prev_jack_value = jack_value;
        jack_value = gpio_pin_get_dt(&jack);
        if (jack_value && !prev_jack_value) {
            LOG_INF("Jack connected!");
        }
        else if (!jack_value && prev_jack_value) {
            LOG_INF("Jack disconnected!");
        }

        state.probe_connected = jack_value;

        k_sleep(K_MSEC(200));
    }

    return;
}

int probe_init(void)
{
    int ret;

    if (!gpio_is_ready_dt(&jack)) {
        LOG_ERR("Error: button device %s is not ready", jack.port->name);
        return -ENODEV;
    }

    ret = gpio_pin_configure_dt(&jack, GPIO_INPUT);
    if (ret != 0) {
        LOG_ERR("Error %d: failed to configure %s pin %d", ret, jack.port->name, jack.pin);
        return -EIO;
    }

    k_thread_create(
        &probe_thread,
        probe_thread_stack,
        K_THREAD_STACK_SIZEOF(probe_thread_stack),
        probe_fn,
        NULL, NULL, NULL,
        K_LOWEST_APPLICATION_THREAD_PRIO,
        0,
        K_NO_WAIT);

    return 0;
}
