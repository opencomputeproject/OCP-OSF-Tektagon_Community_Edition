/*
 * Copyright (c) 2021 AMI
 *
 */

#include <drivers/flash.h>
#include <drivers/spi_nor.h>
#include <gpio/gpio_aspeed.h>
#include <kernel.h>
#include <sys/util.h>
#include <stdlib.h>
#include <string.h>
#include <zephyr.h>
#include <drivers/gpio.h>
#include <drivers/misc/aspeed/pfr_aspeed.h>

#define LOG_MODULE_NAME gpio_api

#include <logging/log.h>
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

static char *GPIO_Devices_List[6] = {
	"GPIO0_A_D",
	"GPIO0_E_H",
	"GPIO0_I_L",
	"GPIO0_M_P",
	"GPIO0_Q_T",
	"GPIO0_U_V",
};

static void GPIO_dump_buf(uint8_t *buf, uint32_t len)
{
	uint32_t i;

	for (i = 0; i < len; i++) {
		printk("%02x ", buf[i]);
		if (i % 16 == 15)
			printk("\n");
	}
	printk("\n");
}

int BMCBootHold(void)
{
	const struct device *gpio_dev = NULL;
	const struct device *dev_m = NULL;

	dev_m = device_get_binding(BMC_SPI_MONITOR);
	spim_rst_flash(dev_m, 1000);
	spim_passthrough_config(dev_m, 0, false);
	spim_ext_mux_config(dev_m, SPIM_MASTER_MODE);
	/* GPIOM5 */
	gpio_dev = device_get_binding("GPIO0_M_P");

	if (gpio_dev == NULL) {
		printk("[%d]Fail to get GPIO0_M_P", __LINE__);
		return -1;
	}

	gpio_pin_configure(gpio_dev, BMC_SRST, GPIO_OUTPUT);

	k_busy_wait(10000); /* 10ms */

	gpio_pin_set(gpio_dev, BMC_SRST, 0);

	k_busy_wait(10000); /* 10ms */

	return 0;
}

int PCHBootHold(void)
{
	const struct device *gpio_dev = NULL;
	const struct device *dev_m = NULL;

	dev_m = device_get_binding(PCH_SPI_MONITOR);
	spim_rst_flash(dev_m, 1000);
	spim_passthrough_config(dev_m, 0, false);
	spim_ext_mux_config(dev_m, SPIM_MASTER_MODE);

	/* GPIOM5 */
	gpio_dev = device_get_binding("GPIO0_M_P");

	if (gpio_dev == NULL) {
		printk("[%d]Fail to get GPIO0_M_P", __LINE__);
		return -1;
	}

	gpio_pin_configure(gpio_dev, CPU0_RST, GPIO_OUTPUT);

	k_busy_wait(10000); /* 10ms */

	gpio_pin_set(gpio_dev, CPU0_RST, 0);

	k_busy_wait(10000); /* 10ms */

	return 0;
}

int BMCBootRelease(void)
{
	const struct device *gpio_dev = NULL;
	const struct device *dev_m = NULL;

	dev_m = device_get_binding(BMC_SPI_MONITOR);
	spim_rst_flash(dev_m, 1000);
	spim_passthrough_config(dev_m, 0, false);
	spim_ext_mux_config(dev_m, SPIM_MONITOR_MODE);

	/* GPIOM5 */
	gpio_dev = device_get_binding("GPIO0_M_P");

	if (gpio_dev == NULL) {
		printk("[%d]Fail to get GPIO0_M_P", __LINE__);
		return -1;
	}

	gpio_pin_configure(gpio_dev, BMC_SRST, GPIO_OUTPUT);

	k_busy_wait(20000); /* 10ms */

	gpio_pin_set(gpio_dev, BMC_SRST, 1);

	k_busy_wait(20000); /* 10ms */

	return 0;
}

int PCHBootRelease(void)
{
	const struct device *gpio_dev = NULL;
	const struct device *dev_m = NULL;

	dev_m = device_get_binding(PCH_SPI_MONITOR);
	spim_rst_flash(dev_m, 1000);
	spim_passthrough_config(dev_m, 0, false);
	spim_ext_mux_config(dev_m, SPIM_MONITOR_MODE);

	/* GPIOM5 */
	gpio_dev = device_get_binding("GPIO0_M_P");

	if (gpio_dev == NULL) {
		printk("[%d]Fail to get GPIO0_M_P", __LINE__);
		return -1;
	}

	gpio_pin_configure(gpio_dev, CPU0_RST, GPIO_OUTPUT);

	k_busy_wait(10000); /* 10ms */

	gpio_pin_set(gpio_dev, CPU0_RST, 1);

	k_busy_wait(10000); /* 10ms */

	return 0;
}

