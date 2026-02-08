#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/devicetree.h>

#define LED0_NODE DT_NODELABEL(led1)

int main(void)
{
   const struct gpio_dt_spec led0 = GPIO_DT_SPEC_GET(LED0_NODE, gpios);

    if (!device_is_ready(led0.port)) {
        return 0;
    }

    gpio_pin_configure_dt(&led0, GPIO_OUTPUT_INACTIVE);

    gpio_pin_set_dt(&led0, 1);

    while (1) {
        k_sleep(K_SECONDS(1));
    		gpio_pin_set_dt(&led0, 1);
        k_sleep(K_SECONDS(1));
    		gpio_pin_set_dt(&led0, 0);
    }
	return 0;
}
