#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/drivers/i2c.h>
#include "sensor_task.h"

K_MSGQ_DEFINE(sensor_msgq, sizeof(struct sensor_msg), 4, 4);

static void sensor_thread(void *a, void *b, void *c)
{
        /* Soft reset SHT3x before driver init check */
    const struct device *i2c = DEVICE_DT_GET(DT_NODELABEL(i2c1));
    const struct device *dev = DEVICE_DT_GET_ANY(sensirion_sht3xd);
    printk("sensor thread\n");

    if (!device_is_ready(dev)) {
        printk("sensor not ready, trying soft reset...\n");
        if (device_is_ready(i2c)) {
            uint8_t reset_cmd[2] = {0x30, 0xA2};
            i2c_write(i2c, reset_cmd, sizeof(reset_cmd), 0x44);
            k_msleep(200);
        }
        int ret = device_init(dev);
        printk("device_init ret=%d\n", ret);
    }

    if (!device_is_ready(dev)) {
        printk("sensor device not ready, giving up\n");
        return;
    }
    printk("sensor device ready\n");

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
        int risk = (h < 40) ? 0 :
                   (h < 50) ? 1 :
                   (h < 60) ? 2 :
                   (h < 70) ? 3 :
                   (h < 80) ? 4 : 5;

        adc_read(adc_bat.dev, &adc_seq);

        int32_t mv = adc_raw;
        adc_raw_to_millivolts_dt(&adc_bat, &mv);
        int battery = (mv < 900)  ? 0 :
                      (mv < 1000) ? 1 :
                      (mv < 1100) ? 2 :
                      (mv < 1200) ? 3 : 4;

        printk("tem = %d hum = %d\n", val_temp.val1, h);
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
