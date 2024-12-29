#include "alarm.h"
#include "state.h"
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/util.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(ct_alarm, CONFIG_CT_ALARM_LOG_LEVEL);

#define PWM_LED_PERIOD NSEC_PER_MSEC

static const struct pwm_dt_spec ui_led = PWM_DT_SPEC_GET(DT_ALIAS(uiled));
static const struct gpio_dt_spec buzzer = GPIO_DT_SPEC_GET(DT_ALIAS(buzzer), gpios);

static struct k_timer alarm_timer;

static uint32_t current_ui_led_pulse = 0;

static void alarm_timer_expiry_function(struct k_timer *timer_id)
{
    if (current_ui_led_pulse > 0) {
        pwm_set_pulse_dt(&ui_led, 0);
        current_ui_led_pulse = 0;
    }
    else {
        pwm_set_pulse_dt(&ui_led, PWM_LED_PERIOD);
        current_ui_led_pulse = PWM_LED_PERIOD;
    }

    gpio_pin_toggle_dt(&buzzer);
}

static void alarm_timer_stop_function(struct k_timer *timer_id)
{
    pwm_set_pulse_dt(&ui_led, 0);
    current_ui_led_pulse = 0;
    gpio_pin_set_dt(&buzzer, 0);
}

int alarm_init(void)
{
    int ret;

    if (!gpio_is_ready_dt(&buzzer)) {
        LOG_ERR("Error: buzzer GPIO is not ready");
        return -ENODEV;
    }
    ret = gpio_pin_configure_dt(&buzzer, GPIO_OUTPUT_INACTIVE);
    if (ret < 0) {
        LOG_ERR("Error %d: failed to configure buzzer gpio", ret);
        return ret;
    }

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


    k_timer_init(&alarm_timer, alarm_timer_expiry_function, alarm_timer_stop_function);
    /* Don't start timer here */

    return 0;
}

void alarm_enable(void)
{
    k_timer_start(&alarm_timer, K_NO_WAIT, K_MSEC(500));
}

void alarm_disable(void)
{
    k_timer_stop(&alarm_timer);
}
