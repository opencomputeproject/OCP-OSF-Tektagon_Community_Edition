/*
 * Copyright (c) 2021 ASPEED Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <sys/printk.h>
#include <string.h>
#include <device.h>
#include <drivers/i3c/i3c.h>

void i3c_spd5118_reg_read(struct i3c_dev_desc *spd5118, uint8_t addr, uint8_t *data, int data_size)
{
	uint8_t out[2] = { addr & GENMASK(5, 0), 0 };
	int ret;

	ret = i3c_jesd403_read(spd5118, out, 2, data, data_size);

	__ASSERT_NO_MSG(!ret);
}
