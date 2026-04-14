#include <zephyr/drivers/sensor.h>
#include "sensor_task.h"

K_MSGQ_DEFINE(sensor_msgq, sizeof(struct sensor_msg), 4, 4);

static void sensor_thread(void *a, void *b, void *c)
{
    const struct device *dev = DEVICE_DT_GET_ANY(sensirion_sht3xd);

    if (!device_is_ready(dev)) {
        return;
    }

    while (1) {
        struct sensor_value val_temp, val_hum;

        sensor_sample_fetch(dev);
        sensor_channel_get(dev, SENSOR_CHAN_AMBIENT_TEMP, &val_temp);
        sensor_channel_get(dev, SENSOR_CHAN_HUMIDITY,     &val_hum);

        struct sensor_msg msg = {
            .temp     = val_temp.val1,
            .humidity = val_hum.val1,
        };

        k_msgq_put(&sensor_msgq, &msg, K_NO_WAIT);
        k_sleep(K_SECONDS(10));
    }
}

K_THREAD_DEFINE(sensor_tid, 1024, sensor_thread, NULL, NULL, NULL, 5, 0, 0);
