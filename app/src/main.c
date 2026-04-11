#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/sys/printk.h>
#include "display.h"
#include "ui.h"

#define LED0_NODE DT_ALIAS(led0)
int main(void)
{
// __asm__ volatile("bkpt 0");
	// ret = device_init(epd);
    const struct gpio_dt_spec led0 = GPIO_DT_SPEC_GET(LED0_NODE, gpios);

    if (!device_is_ready(led0.port)) {
        printk("LED port not ready\n");
        return 0;
    }

    display_init();
	ui_set_temperature(-1);
	ui_set_humidity(12);
	ui_render();

    gpio_pin_configure_dt(&led0, GPIO_OUTPUT_INACTIVE);
    printk("Init ok\n");

    while (1) {
        gpio_pin_set_dt(&led0, 1);
        k_sleep(K_SECONDS(1));
        gpio_pin_set_dt(&led0, 0);
        k_sleep(K_SECONDS(1));
    }
}

