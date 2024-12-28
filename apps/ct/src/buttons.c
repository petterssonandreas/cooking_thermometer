#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/printk.h>
#include <errno.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(ct_buttons, CONFIG_CT_BUTTONS_LOG_LEVEL);

#define NUM_BUTTONS (3)

static const struct gpio_dt_spec buttons[NUM_BUTTONS] = {
    GPIO_DT_SPEC_GET_OR(DT_ALIAS(swa), gpios, {0}),
    GPIO_DT_SPEC_GET_OR(DT_ALIAS(swb), gpios, {0}),
    GPIO_DT_SPEC_GET_OR(DT_ALIAS(swc), gpios, {0})
};

static K_THREAD_STACK_DEFINE(buttons_thread_stack, 1024);

static struct k_thread buttons_thread;

static int buttons_pressed[NUM_BUTTONS] = {0};

bool button_is_pressed(size_t button_index)
{
    if (button_index < 0 || button_index >= ARRAY_SIZE(buttons_pressed)) {
        return -EINVAL;
    }

    return (buttons_pressed[button_index] != 0);
}

static void buttons_fn(void *p1, void *p2, void *p3)
{
    UNUSED(p1);
    UNUSED(p2);
    UNUSED(p3);

    static int prev_button_state[NUM_BUTTONS] = {0};
    static int button_state[NUM_BUTTONS] = {0};

    LOG_INF("Button thread started");

    while (true) {
        // memcpy(prev_button_state, button_state, sizeof(prev_button_state));

        for (size_t i = 0; i < ARRAY_SIZE(buttons); i++) {
            prev_button_state[i] = button_state[i];
            button_state[i] = gpio_pin_get_dt(&buttons[i]);

            // LOG_INF("button_state[%d] = %d", i, button_state[i]);

            if ((buttons_pressed[i] == 0) && (prev_button_state[i] == 1) && (button_state[i] == 1)) {
                // Was not pressed before, but has now been pressed long enough
                // TODO: Trigger event? Callback?
                buttons_pressed[i] = 1;
                LOG_INF("Button %d pressed", i);
            }
            else if ((buttons_pressed[i] == 1) && (prev_button_state[i] == 0) && (button_state[i] == 0)) {
                // Was pressed before, but has now been not pressed long enough
                // TODO: Trigger event? Callback?
                buttons_pressed[i] = 0;
                LOG_INF("Button %d released", i);
            }
        }

        k_sleep(K_MSEC(20));
    }

    return;
}

int buttons_init(void)
{
    int ret;

    for (size_t i = 0; i < ARRAY_SIZE(buttons); i++) {
        if (!gpio_is_ready_dt(&buttons[i])) {
            LOG_ERR("Error: button device %s is not ready", buttons[i].port->name);
            return -ENODEV;
        }

        ret = gpio_pin_configure_dt(&buttons[i], GPIO_INPUT);
        if (ret != 0) {
            LOG_ERR("Error %d: failed to configure %s pin %d", ret, buttons[i].port->name, buttons[i].pin);
            return -EIO;
        }
    }

    k_thread_create(
        &buttons_thread,
        buttons_thread_stack,
        K_THREAD_STACK_SIZEOF(buttons_thread_stack),
        buttons_fn,
        NULL, NULL, NULL,
        K_LOWEST_APPLICATION_THREAD_PRIO,
        0,
        K_NO_WAIT);

    return 0;
}
