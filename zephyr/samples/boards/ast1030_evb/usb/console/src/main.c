/*
 * Copyright (c) 2021 Aspeed Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <sys/printk.h>
#include <sys/util.h>
#include <string.h>
#include <usb/usb_device.h>
#include <drivers/uart.h>

void main(void)
{
	const struct device *dev;
	int count = 0;

	if (strlen(CONFIG_UART_CONSOLE_ON_DEV_NAME) != strlen("CDC_ACM_0") ||
	    strncmp(CONFIG_UART_CONSOLE_ON_DEV_NAME, "CDC_ACM_0",
		    strlen(CONFIG_UART_CONSOLE_ON_DEV_NAME))) {
		printk("Error: Console device name is not USB ACM\n");
		return;
	}

	dev = device_get_binding(CONFIG_UART_CONSOLE_ON_DEV_NAME);
	if (!dev) {
		printk("Error: Device get binding failed\n");
		return;
	}

	if (usb_enable(NULL)) {
		printk("Error: USB enable failed\n");
		return;
	}

	/* Poll if the RTS flag was set */
	uint32_t dtr = 0;

	while (!dtr) {
		uart_line_ctrl_get(dev, UART_LINE_CTRL_RTS, &dtr);
	}

	while (1) {
		printk("%s: %s: Hello World! count: %d\n", CONFIG_ARCH, CONFIG_BOARD, count++);
		k_sleep(K_SECONDS(1));
	}
}
