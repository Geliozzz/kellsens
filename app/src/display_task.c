#include "display_task.h"
#include "sensor_task.h"
#include "ui.h"

static void display_thread(void *a, void *b, void *c)
{
    static struct sensor_msg last_msg = { .temp = -2, .humidity = 23, .dew_risk = 0, .battery = 4 };

    while (1) {
        k_sleep(K_SECONDS(30));

        struct sensor_msg msg;
        while (k_msgq_get(&sensor_msgq, &msg, K_NO_WAIT) == 0) {
            last_msg = msg;
        }

        ui_set_temperature(last_msg.temp);
        ui_set_humidity(last_msg.humidity);
        ui_set_dew_risk(last_msg.dew_risk);
        ui_set_battery(last_msg.battery);
        printk("render: temp=%d hum=%d risk=%d bat=%d\n",
       last_msg.temp,
       last_msg.humidity,
       last_msg.dew_risk,
       last_msg.battery);
        ui_render();
    }
}

K_THREAD_DEFINE(display_tid, 1024, display_thread, NULL, NULL, NULL, 7, 0, 0);
