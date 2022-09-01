/*
 * Copyright (c) 2021 ASPEED Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr.h>
#include <sys/printk.h>
#include <string.h>
#include <device.h>
#include <drivers/misc/aspeed/snoop_aspeed.h>

void main(void)
{
	int i, rc;
	const struct device *snoop_dev;
	uint8_t snoop_data[SNOOP_CHANNEL_NUM];

	snoop_dev = device_get_binding(DT_LABEL(DT_NODELABEL(snoop)));
	if (!snoop_dev) {
		printk("No snoop device found\n");
		return;
	}

	while (1) {
		for (i = 0; i < SNOOP_CHANNEL_NUM; ++i) {
			rc = snoop_aspeed_read(snoop_dev, i, &snoop_data[i], true);
			if (rc == 0)
				printk("snoop[%d] = 0x%02x\n", i, snoop_data[i]);
		}
	}
}
