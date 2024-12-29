#include "adc.h"
#include "display.h"
#include "state.h"

#include <stdlib.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/util.h>


#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(ct_adc, CONFIG_CT_ADC_LOG_LEVEL);

static const struct device *const adc1 = DEVICE_DT_GET(DT_ALIAS(adc1));

static const struct gpio_dt_spec jack = GPIO_DT_SPEC_GET_OR(DT_ALIAS(jack_connected), gpios, {0});

/* Define ADC resolution and channel */
#define ADC_RESOLUTION 12
#define ADC_BATT_CHANNEL_ID 2
#define ADC_TEMP_CHANNEL_ID 10

/* ADC sequence configuration for battery */
static struct adc_channel_cfg adc_batt_cfg = {
    .gain             = ADC_GAIN_1,
    .reference        = ADC_REF_INTERNAL,
    .acquisition_time = ADC_ACQ_TIME_MAX,
    .channel_id       = ADC_BATT_CHANNEL_ID,
    .differential     = 0,
};

/* ADC sequence configuration for temperature */
static struct adc_channel_cfg adc_temp_cfg = {
    .gain             = ADC_GAIN_1,
    .reference        = ADC_REF_INTERNAL,
    .acquisition_time = ADC_ACQ_TIME_MAX,
    .channel_id       = ADC_TEMP_CHANNEL_ID,
    .differential     = 0,
};

typedef struct CurveValue_s
{
    int16_t raw_reading;
    int16_t temperature;
} CurveValue_t;

static CurveValue_t TemperatureCurve[] =
{
    {.raw_reading=3790, .temperature=-20},
    {.raw_reading=3046, .temperature=4},
    {.raw_reading=2226, .temperature=22},
    {.raw_reading=2120, .temperature=24},
    {.raw_reading=2016, .temperature=26},
    {.raw_reading=1724, .temperature=32},
    {.raw_reading=1664, .temperature=34},
    {.raw_reading=1407, .temperature=40},
    {.raw_reading=1104, .temperature=49},
    {.raw_reading=994, .temperature=52},
    {.raw_reading=812, .temperature=58},
    {.raw_reading=782, .temperature=59},
    {.raw_reading=584, .temperature=69},
    {.raw_reading=535, .temperature=71},
    {.raw_reading=474, .temperature=75},
    {.raw_reading=394, .temperature=81},
    {.raw_reading=370, .temperature=83},
    {.raw_reading=334, .temperature=86},
    {.raw_reading=321, .temperature=88},
};


static int16_t interpolate_temperature(int16_t raw)
{
    if (raw > TemperatureCurve[0].raw_reading) {
        return INT16_MIN;
    }
    if (raw < TemperatureCurve[ARRAY_SIZE(TemperatureCurve) - 1].raw_reading) {
        return INT16_MAX;
    }

    for (size_t i = 0; i < ARRAY_SIZE(TemperatureCurve) - 1; i++)
    {
        if ((raw <= TemperatureCurve[i].raw_reading) && (raw > TemperatureCurve[i + 1].raw_reading)) {
            int32_t raw_range = abs(TemperatureCurve[i + 1].raw_reading - TemperatureCurve[i].raw_reading);
            int32_t temp_range = abs(TemperatureCurve[i + 1].temperature - TemperatureCurve[i].temperature);
            int32_t offset_into_raw = abs(raw - TemperatureCurve[i].raw_reading);
            return ((offset_into_raw * temp_range) / raw_range) + TemperatureCurve[i].temperature;
        }
    }

    /* Should not get here */
    return INT16_MIN;
}

static K_THREAD_STACK_DEFINE(adc_temperature_thread_stack, 1024);

static struct k_thread adc_temperature_thread;

static void adc_temperature_fn(void *p1, void *p2, void *p3)
{
    UNUSED(p1);
    UNUSED(p2);
    UNUSED(p3);

    int prev_jack_value = 0;
    int jack_value = 0;

    while (true) {
        k_sleep(K_MSEC(1500));

        /* Check if probe connected */
        prev_jack_value = jack_value;
        jack_value = gpio_pin_get_dt(&jack);
        if (jack_value && !prev_jack_value) {
            LOG_INF("Jack connected!");
        }
        else if (!jack_value && prev_jack_value) {
            LOG_INF("Jack disconnected!");
        }

        if (jack_value && prev_jack_value) {
            /* Probe is connected and has been for long enough to get stable reading */
            state.probe_connected = true;

            int16_t temperature;
            int ret = adc_get_temperature(&temperature);
            if (ret) {
                LOG_ERR("Failed to get temp reading");
            }
            else {
                state.temperature = temperature;
            }
        }
        else {
            state.probe_connected = false;
            state.temperature = INT16_MIN;
        }
    }
}

int adc_init(void)
{
    int ret;

    /* Get the ADC device */
    if (!device_is_ready(adc1)) {
        LOG_ERR("ADC device not found");
        return -ENODEV;
    }

    /* Configure the ADC channel for battery voltage */
    ret = adc_channel_setup(adc1, &adc_batt_cfg);
    if (ret < 0) {
        LOG_ERR("ADC channel setup for battery failed with error %d", ret);
        return -ENODEV;
    }

    /* Configure the ADC channel for temperature voltage */
    ret = adc_channel_setup(adc1, &adc_temp_cfg);
    if (ret < 0) {
        LOG_ERR("ADC channel setup for temperature failed with error %d", ret);
        return -ENODEV;
    }

    /* Also configure Probe GPIO, to know if it is connected */
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
        &adc_temperature_thread,
        adc_temperature_thread_stack,
        K_THREAD_STACK_SIZEOF(adc_temperature_thread_stack),
        adc_temperature_fn,
        NULL, NULL, NULL,
        K_LOWEST_APPLICATION_THREAD_PRIO,
        0,
        K_NO_WAIT);

    return 0;
}

int adc_get_battery_voltage(int16_t *vbat_mv)
{
    int ret;
    int16_t sample_buffer;
    struct adc_sequence sequence = {
        .channels    = BIT(ADC_BATT_CHANNEL_ID),
        .buffer      = &sample_buffer,
        .buffer_size = sizeof(sample_buffer),
        .resolution  = ADC_RESOLUTION,
    };

    ret = adc_read(adc1, &sequence);
    if (ret < 0) {
        LOG_ERR("ADC read failed with error %d", ret);
        return ret;
    }

    int32_t valp = sample_buffer;
    ret = adc_raw_to_millivolts(3300, adc_batt_cfg.gain, sequence.resolution, &valp);
    if (ret < 0) {
        LOG_ERR("ADC raw to mV failed with error %d", ret);
        return ret;
    }

    valp *= 2; /* Take *2 because of voltage division */
    LOG_DBG("ADC sample value: %d mV (raw: %d)", valp, sample_buffer);
    *vbat_mv = valp;
    return 0;
}

int adc_get_temperature(int16_t *temperature)
{
    int ret;
    int16_t sample_buffer;
    struct adc_sequence sequence = {
        .channels    = BIT(ADC_TEMP_CHANNEL_ID),
        .buffer      = &sample_buffer,
        .buffer_size = sizeof(sample_buffer),
        .resolution  = ADC_RESOLUTION,
    };

    ret = adc_read(adc1, &sequence);
    if (ret < 0) {
        LOG_ERR("ADC read failed with error %d", ret);
        return ret;
    }

    int16_t interpolated_temp = interpolate_temperature(sample_buffer);
    LOG_DBG("ADC temperature sample value: %d C (raw: %d)", interpolated_temp, sample_buffer);
    *temperature = interpolated_temp;
    return 0;
}
