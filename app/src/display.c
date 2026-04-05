#include "display.h"

#include <stdint.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

#define DC_PIN 0
#define RST_PIN 1
#define BUSY_PIN 5
#define CS_PIN 4

#define EPD_WIDTH 200
#define EPD_HEIGHT 200
#define EPD_BUF_SIZE (EPD_WIDTH * EPD_HEIGHT / 8)

static const struct device *gpio_dev = DEVICE_DT_GET(DT_NODELABEL(gpioc));
static const struct device *gpio_dev_a = DEVICE_DT_GET(DT_NODELABEL(gpioa));
static const struct device *spi_dev = DEVICE_DT_GET(DT_NODELABEL(spi1));

static struct spi_config spi_cfg = {
    .frequency = 1000000,
    .operation = SPI_WORD_SET(8) | SPI_TRANSFER_MSB,
    .slave = 0,
};

static void cs_low(void) { gpio_pin_set(gpio_dev_a, CS_PIN, 0); }

static void cs_high(void) { gpio_pin_set(gpio_dev_a, CS_PIN, 1); }

static void dc_cmd(void) { gpio_pin_set(gpio_dev, DC_PIN, 0); }

static void dc_data(void) { gpio_pin_set(gpio_dev, DC_PIN, 1); }

static int spi_write_byte(uint8_t byte) {
    struct spi_buf buf = {
        .buf = &byte,
        .len = 1,
    };
    struct spi_buf_set tx = {
        .buffers = &buf,
        .count = 1,
    };

    return spi_write(spi_dev, &spi_cfg, &tx);
}

static void epd_cmd(uint8_t cmd) {
    dc_cmd();
    cs_low();
    (void)spi_write_byte(cmd);
    cs_high();
}

static void epd_data(uint8_t data) {
    dc_data();
    cs_low();
    (void)spi_write_byte(data);
    cs_high();
}

static void epd_reset(void) {
    gpio_pin_set(gpio_dev, RST_PIN, 1);
    k_msleep(10);
    gpio_pin_set(gpio_dev, RST_PIN, 0);
    k_msleep(10);
    gpio_pin_set(gpio_dev, RST_PIN, 1);
    k_msleep(20);
}

static void epd_wait_busy_timeout(int timeout_ms) {
    int elapsed = 0;

    while (elapsed < timeout_ms) {
        int v = gpio_pin_get(gpio_dev, BUSY_PIN);

        if (v > 0) {
            return;
        }

        k_msleep(5);
        elapsed += 5;
    }

    printk("BUSY timeout, continue anyway\n");
}

static void epd_set_window_full(void) {
    epd_cmd(0x44); /* SET_RAM_X_ADDRESS_START_END_POSITION */
    epd_data(0x00);
    epd_data((EPD_WIDTH / 8) - 1); /* 200/8 = 25 bytes => last = 24 = 0x18 */

    epd_cmd(0x45); /* SET_RAM_Y_ADDRESS_START_END_POSITION */
    epd_data((EPD_HEIGHT - 1) & 0xFF);
    epd_data(((EPD_HEIGHT - 1) >> 8) & 0xFF);
    epd_data(0x00);
    epd_data(0x00);
}

static void epd_set_cursor(uint8_t x, uint16_t y) {
    epd_cmd(0x4E); /* SET_RAM_X_ADDRESS_COUNTER */
    epd_data(x);

    epd_cmd(0x4F); /* SET_RAM_Y_ADDRESS_COUNTER */
    epd_data(y & 0xFF);
    epd_data((y >> 8) & 0xFF);
}

static void epd_init_panel(void) {
    epd_reset();

    epd_cmd(0x12); /* SWRESET */
    k_msleep(200);

    epd_cmd(0x01);                            /* Driver output control */
    epd_data((EPD_HEIGHT - 1) & 0xFF);        /* 199 = 0xC7 */
    epd_data(((EPD_HEIGHT - 1) >> 8) & 0xFF); /* 0x00 */
    epd_data(0x00);

    epd_cmd(0x11);  /* Data entry mode */
    epd_data(0x01); /* X+, Y+ */

    epd_set_window_full();
    epd_set_cursor(0x00, 0x0000);

    epd_cmd(0x3C); /* Border waveform */
    epd_data(0x05);

    epd_cmd(0x18); /* Temperature sensor control */
    epd_data(0x80);

    k_msleep(20);
}

static void epd_clear_bw(uint8_t value) {
    epd_set_window_full();
    epd_set_cursor(0x00, 0x0000);

    epd_cmd(0x24); /* Write BW RAM */
    for (int i = 0; i < EPD_BUF_SIZE; i++) {
        epd_data(value);
    }
}

static void epd_draw_test_pattern(void) {
    uint8_t line[EPD_WIDTH / 8];

    epd_set_window_full();
    epd_set_cursor(0x00, 0x0000);

    epd_cmd(0x24); /* Write BW RAM */

    for (int y = 0; y < EPD_HEIGHT; y++) {
        for (int i = 0; i < EPD_WIDTH / 8; i++) {
            line[i] = 0xFF; /* white */
        }

        if (y == 0 || y == (EPD_HEIGHT - 1)) {
            for (int i = 0; i < EPD_WIDTH / 8; i++) {
                line[i] = 0x00; /* black full line */
            }
        } else {
            line[0] = 0x00;                   /* left border */
            line[(EPD_WIDTH / 8) - 1] = 0x00; /* right border */
        }

        /* Небольшая черная полоса по центру */
        if (y > 90 && y < 110) {
            for (int i = 8; i < 17; i++) {
                line[i] = 0x00;
            }
        }

        for (int i = 0; i < EPD_WIDTH / 8; i++) {
            epd_data(line[i]);
        }
    }
}

static void epd_refresh(void) {
    epd_cmd(0x22); /* Display update control */
    epd_data(0xF7);

    epd_cmd(0x20); /* Master activation */
    k_msleep(2000);
}

void display_init(void) {
    if (!device_is_ready(gpio_dev)) {
        printk("gpio not ready\n");
        return;
    }

    if (!device_is_ready(spi_dev)) {
        printk("spi not ready\n");
        return;
    }

    if (gpio_pin_configure(gpio_dev, DC_PIN, GPIO_OUTPUT_ACTIVE) != 0) {
        printk("DC pin configure failed\n");
        return;
    }

    if (gpio_pin_configure(gpio_dev, RST_PIN, GPIO_OUTPUT_ACTIVE) != 0) {
        printk("RST pin configure failed\n");
        return;
    }

    if (gpio_pin_configure(gpio_dev_a, CS_PIN, GPIO_OUTPUT_ACTIVE) != 0) {
        printk("CS pin configure failed\n");
        return;
    }

    if (gpio_pin_configure(gpio_dev, BUSY_PIN, GPIO_INPUT | GPIO_PULL_UP) !=
        0) {
        printk("BUSY pin configure failed\n");
        return;
    }

    cs_high();
    dc_data();
    gpio_pin_set(gpio_dev, RST_PIN, 1);

    printk("EPD init start, BUSY=%d\n", gpio_pin_get(gpio_dev, BUSY_PIN));

    epd_init_panel();

    epd_wait_busy_timeout(500);

    epd_clear_bw(0xFF);
    epd_refresh();
    k_msleep(1000);

    epd_draw_test_pattern();
    epd_refresh();

    printk("EPD init done, BUSY=%d\n", gpio_pin_get(gpio_dev, BUSY_PIN));
}
