#pragma once
#include <zephyr/kernel.h>

struct sensor_msg {
    int temp;      /* °C, integer */
    int humidity;  /* %, integer */
};

extern struct k_msgq sensor_msgq;

void sensor_task_start(void);
