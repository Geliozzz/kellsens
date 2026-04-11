#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/sys/printk.h>
#include <zephyr/drivers/display.h>
#include "display.h"

#define LED0_NODE DT_ALIAS(led0)
#define EPD_NODE DT_NODELABEL(epd_mipi)
int main(void)
{
    const struct gpio_dt_spec led0 = GPIO_DT_SPEC_GET(LED0_NODE, gpios);

    if (!device_is_ready(led0.port)) {
        printk("LED port not ready\n");
        return 0;
    }

    	const struct device *epd = DEVICE_DT_GET(EPD_NODE);
	int ret;

	// k_msleep(3000);
	printk("late init start\n");
__asm__ volatile("bkpt 0");
	ret = device_init(epd);
	printk("device_init ret=%d\n", ret);

    gpio_pin_configure_dt(&led0, GPIO_OUTPUT_INACTIVE);

    // display_init();


    printk("Init ok\n");

    while (1) {
        gpio_pin_set_dt(&led0, 1);
        k_sleep(K_SECONDS(1));
        gpio_pin_set_dt(&led0, 0);
        k_sleep(K_SECONDS(1));
    }
}

