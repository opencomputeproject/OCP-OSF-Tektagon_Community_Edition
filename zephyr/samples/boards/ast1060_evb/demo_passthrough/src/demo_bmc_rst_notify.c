/*
 * Copyright (c) 2021 ASPEED Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <sys/printk.h>
#include <drivers/misc/aspeed/pfr_aspeed.h>
#include "demo_gpio.h"

static struct gpio_callback gpio_cb;
struct k_work rst_work;

static void gpioo3_callback(const struct device *dev,
		       struct gpio_callback *gpio_cb, uint32_t pins)
{
	/* reset BMC first */
	pfr_bmc_rst_enable_ctrl(true);

	k_work_submit(&rst_work);

	ARG_UNUSED(*gpio_cb);
	ARG_UNUSED(pins);
}

void rst_gpio_event_register(void)
{
	int ret;
	const struct device *gpio_dev = device_get_binding(GPIOO3_DEV_NAME);

	if (!gpio_dev) {
		printk("demo_err: cannot get device, %s.\n", gpio_dev->name);
		return;
	}

	ret = gpio_pin_interrupt_configure(gpio_dev, GPIOO3_PIN_IN, GPIO_INT_DISABLE);
	if (ret != 0) {
		printk("demo_err: cannot configure GPIO INT(disabled).\n");
		return;
	}

	ret = gpio_pin_configure(gpio_dev, GPIOO3_PIN_IN, GPIO_INPUT);
	if (ret != 0) {
		printk("demo_err: cannot configure gpio device.\n");
		return;
	}

	gpio_init_callback(&gpio_cb, gpioo3_callback, BIT(GPIOO3_PIN_IN));
	ret = gpio_add_callback(gpio_dev, &gpio_cb);
	if (ret != 0) {
		printk("demo_err: cannot add gpio callback.\n");
		return;
	}

	ret = gpio_pin_interrupt_configure(gpio_dev, GPIOO3_PIN_IN, GPIO_INT_EDGE_TO_INACTIVE);
	if (ret != 0) {
		printk("demo_err: cannot configure GPIO INT(1->0).\n");
		return;
	}
}

