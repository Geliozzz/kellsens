#include "display_task.h"
#include "sensor_task.h"
#include "ui.h"

static void display_thread(void *a, void *b, void *c)
{
    static int last_temp     = 8;
    static int last_humidity = 72;

    while (1) {
        k_sleep(K_SECONDS(30));

        struct sensor_msg msg;
        while (k_msgq_get(&sensor_msgq, &msg, K_NO_WAIT) == 0) {
            last_temp     = msg.temp;
            last_humidity = msg.humidity;
        }

        ui_set_temperature(last_temp);
        ui_set_humidity(last_humidity);
        ui_render();
    }
}

K_THREAD_DEFINE(display_tid, 1024, display_thread, NULL, NULL, NULL, 7, 0, 0);
