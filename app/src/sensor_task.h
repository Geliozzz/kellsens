#pragma once
#include <zephyr/kernel.h>

struct sensor_msg {
    int temp;      /* °C, integer */
    int humidity;  /* %, integer */
    int dew_risk;  /* 0..5 */
    int battery;   /* 0..4 */
};

extern struct k_msgq sensor_msgq;

void sensor_task_start(void);
