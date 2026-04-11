#include "display.h"

#include <stdint.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/mipi_dbi.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

#define MIPI_DBI_BUS_NODE DT_NODELABEL(mipi_dbi0)
#define MIPI_DBI_DEV_NODE DT_NODELABEL(epd_mipi)
#define BUSY_NODE         DT_NODELABEL(epd_busy)

static const struct device *dbi_dev = DEVICE_DT_GET(MIPI_DBI_BUS_NODE);

// static const struct mipi_dbi_config dbi_config =
// 	MIPI_DBI_CONFIG_DT(
// 		MIPI_DBI_DEV_NODE,
// 		SPI_OP_MODE_MASTER | SPI_TRANSFER_MSB | SPI_WORD_SET(8),
// 		0
// 	);

static const struct mipi_dbi_config dbi_config =
	MIPI_DBI_CONFIG_DT(
		MIPI_DBI_DEV_NODE,
		SPI_OP_MODE_MASTER |
		SPI_TRANSFER_MSB |
		SPI_WORD_SET(8) |
		SPI_HALF_DUPLEX,
		0
	);

#define EPD_WIDTH    200
#define EPD_HEIGHT   200
#define EPD_BUF_SIZE (EPD_WIDTH * EPD_HEIGHT / 8)

static uint8_t epd_buf[EPD_BUF_SIZE] __aligned(4);

static void epd_wait_busy_timeout(int timeout_ms)
{
	ARG_UNUSED(timeout_ms);
	k_msleep(400);
}

void display_init2(void)
{
	if (!device_is_ready(dbi_dev)) {
		printk("mipi dbi not ready\n");
		return;
	}

	printk("before reset\n");

	printk("done\n");
}

void display_init(void)
{
	if (!device_is_ready(dbi_dev)) {
		printk("mipi dbi not ready\n");
		return;
	}

	printk("EPD_BUF_SIZE=%d\n", EPD_BUF_SIZE);
	// mipi_dbi_rese (dbi_dev, 100);

	printk("cmd 0x12\n");
	// static uint8_t epd_buf[EPD_BUF_SIZE];
	k_msleep(200);
	int ret = mipi_dbi_command_write(dbi_dev, &dbi_config, 0x12, NULL, 0);
	printk("ret=%d\n", ret);
	k_msleep(200);

	memset(epd_buf, 0x00, EPD_BUF_SIZE);

 		uint8_t val = 0x03;
    ret = mipi_dbi_command_write(dbi_dev, &dbi_config, 0x11, &val, 1);
    printk("0x11 ret=%d\n", ret);


	/* X window: 0..24 */
	uint8_t xwin[2] = {0x00, 0x18};
	ret = mipi_dbi_command_write(dbi_dev, &dbi_config, 0x44, xwin, 2);
	printk("0x44 ret=%d\n", ret);
k_msleep(200);
	/* Y window: 0..199 */
	uint8_t ywin[4] = {0x00, 0x00, 0xC7, 0x00};
	ret = mipi_dbi_command_write(dbi_dev, &dbi_config, 0x45, ywin, 4);
	printk("0x45 ret=%d\n", ret);

	/* X counter = 0 */
	uint8_t xcnt = 0x00;
	ret = mipi_dbi_command_write(dbi_dev, &dbi_config, 0x4E, &xcnt, 1);
	printk("0x4E ret=%d\n", ret);

	/* Y counter = 0 */
	uint8_t ycnt[2] = {0x00, 0x00};
	ret = mipi_dbi_command_write(dbi_dev, &dbi_config, 0x4F, ycnt, 2);
	printk("0x4F ret=%d\n", ret);
}

void epd_clear(void)
{
	memset(epd_buf, 0xFF, EPD_BUF_SIZE);
}

void display_refresh(void)
{
	/* Reset address counters so refresh is idempotent */
	uint8_t xcnt = 0x00;
	int ret = mipi_dbi_command_write(dbi_dev, &dbi_config, 0x4E, &xcnt, 1);
	printk("0x4E ret=%d\n", ret);

	uint8_t ycnt[2] = {0x00, 0x00};
	ret = mipi_dbi_command_write(dbi_dev, &dbi_config, 0x4F, ycnt, 2);
	printk("0x4F ret=%d\n", ret);

	ret = mipi_dbi_command_write(dbi_dev, &dbi_config, 0x24, epd_buf, EPD_BUF_SIZE);
	printk("after 0x24 ret=%d\n", ret);
k_msleep(500);
printk("still alive after 0x24\n");

	/* Display refresh */
	ret = mipi_dbi_command_write(dbi_dev, &dbi_config, 0x20, NULL, 0);
    printk("0x20 ret=%d\n", ret);

	printk("done\n");
}

uint8_t *display_get_buf(void)
{
	return epd_buf;
}
