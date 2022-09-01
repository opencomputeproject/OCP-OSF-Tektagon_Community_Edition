/*
 * Copyright (c) 2021 ASPEED Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr.h>
#include <sys/printk.h>
#include <string.h>
#include <device.h>
#include <drivers/misc/aspeed/mbox_aspeed.h>

void main(void)
{
	int i, rc;
	const struct device *mbox_dev;
	uint8_t mbox_data[MBX_DAT_REG_NUM];

	mbox_dev = device_get_binding(DT_LABEL(DT_NODELABEL(mbox)));
	if (!mbox_dev) {
		printk("No mbox device found\n");
		return;
	}

	while (1) {
		rc = mbox_aspeed_read(mbox_dev, mbox_data, MBX_DAT_REG_NUM, 0);
		if (rc)
			continue;

		for (i = 0; i < MBX_DAT_REG_NUM; ++i)
			printk("MBX[%d]=0x%02x\n", i, mbox_data[i]);
	}
}
