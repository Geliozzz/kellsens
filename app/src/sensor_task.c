#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/adc.h>
#include "sensor_task.h"

K_MSGQ_DEFINE(sensor_msgq, sizeof(struct sensor_msg), 4, 4);

static void sensor_thread(void *a, void *b, void *c)
{
    const struct device *dev = DEVICE_DT_GET_ANY(sensirion_sht3xd);

    if (!device_is_ready(dev)) {
        return;
    }

    static const struct adc_dt_spec adc_bat =
        ADC_DT_SPEC_GET(DT_PATH(zephyr_user));

    int16_t adc_raw = 0;
    struct adc_sequence adc_seq = {
        .buffer      = &adc_raw,
        .buffer_size = sizeof(adc_raw),
    };

    adc_channel_setup_dt(&adc_bat);
    adc_sequence_init_dt(&adc_bat, &adc_seq);

    while (1) {
        struct sensor_value val_temp, val_hum;
        printk("sensor task\n");
        sensor_sample_fetch(dev);
        sensor_channel_get(dev, SENSOR_CHAN_AMBIENT_TEMP, &val_temp);
        sensor_channel_get(dev, SENSOR_CHAN_HUMIDITY,     &val_hum);

        int h = val_hum.val1;
        h = 75;
        int risk = (h < 40) ? 0 :
                   (h < 50) ? 1 :
                   (h < 60) ? 2 :
                   (h < 70) ? 3 :
                   (h < 80) ? 4 : 5;

        adc_read(adc_bat.dev, &adc_seq);

        int32_t mv = adc_raw;
        adc_raw_to_millivolts_dt(&adc_bat, &mv);
        mv = 1001;
        int battery = (mv < 900)  ? 0 :
                      (mv < 1000) ? 1 :
                      (mv < 1100) ? 2 :
                      (mv < 1200) ? 3 : 4;

        risk = 1;
        battery = 3;

        struct sensor_msg msg = {
            .temp     = val_temp.val1,
            .humidity = val_hum.val1,
            .dew_risk = risk,
            .battery  = battery,
        };

        k_msgq_put(&sensor_msgq, &msg, K_NO_WAIT);
        k_sleep(K_SECONDS(10));
    }
}

K_THREAD_DEFINE(sensor_tid, 1024, sensor_thread, NULL, NULL, NULL, 5, 0, 0);
