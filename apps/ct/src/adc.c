#include "cttime.h"

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/sys/util.h>


#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(ct_adc, CONFIG_CT_ADC_LOG_LEVEL);

static const struct device *const adc1 = DEVICE_DT_GET(DT_ALIAS(adc1));

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

int adc_get_temperature_raw(int16_t *temperature_raw)
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

    *temperature_raw = sample_buffer;
    return 0;
}
