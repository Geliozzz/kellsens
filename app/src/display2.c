#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/devicetree.h>

#define EPD_NODE DT_NODELABEL(epd)

extern int device_init(const struct device *dev);

void display_init2(void)
{
        // const struct device *epd = DEVICE_DT_GET(EPD_NODE);

    printk("before epd init\n");
    // int ret = device_init(epd);
    // printk("after epd init ret\n");
    const struct device *epd = DEVICE_DT_GET(EPD_NODE);

    if (!device_is_ready(epd)) {
        printk("epd not ready yet, calling device_init()\n");
    }

    printk("before epd init\n");
    int ret = device_init(epd);
    printk("after epd init ret=%d\n", ret);    
}
