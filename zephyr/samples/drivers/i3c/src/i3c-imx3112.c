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

void i3c_imx3112_reg_read(struct i3c_dev_desc *imx3112, uint8_t addr, uint8_t *data,
				 int data_size)
{
	uint8_t out[2];
	int ret;

	out[0] = addr & GENMASK(6, 0);
	out[1] = addr >> 7;
	ret = i3c_jesd403_read(imx3112, out, 2, data, data_size);
	__ASSERT_NO_MSG(!ret);
}

void i3c_imx3112_reg_write(struct i3c_dev_desc *imx3112, uint8_t addr, uint8_t *data,
				  int data_size)
{
	uint8_t out[2];
	int ret;

	out[0] = addr & GENMASK(6, 0);
	out[1] = addr >> 7;
	ret = i3c_jesd403_write(imx3112, out, 2, data, data_size);
	__ASSERT_NO_MSG(!ret);
}

void i3c_imx3112_select(struct i3c_dev_desc *imx3112, int chan)
{
	int ret;
	uint8_t data[2];

	if (chan) {
		data[0] = 0x80;
		data[1] = 0x80;
	} else {
		data[0] = 0x40;
		data[1] = 0x40;
	}

	if (imx3112->info.i2c_mode) {
		ret = i3c_i2c_write(imx3112, 0x40, data, 2);
		__ASSERT_NO_MSG(!ret);
	} else {
		i3c_imx3112_reg_write(imx3112, 0x40, data, 2);
	}
}
