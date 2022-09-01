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

#define IMX3102_PORT_CONF		0x40
#define   IMX3102_PORT_CONF_M1_EN	BIT(7)
#define   IMX3102_PORT_CONF_S_EN	BIT(6)
#define IMX3102_PORT_SEL		0x41
#define   IMX3102_PORT_SEL_M1		BIT(7)
#define   IMX3102_PORT_SEL_S_EN		BIT(6)

void i3c_imx3102_reg_read(struct i3c_dev_desc *imx3102, uint8_t addr, uint8_t *data, int data_size)
{
	uint8_t out[2];
	int ret;

	out[0] = addr;
	out[1] = 0;
	ret = i3c_jesd403_read(imx3102, out, 2, data, data_size);
	__ASSERT_NO_MSG(!ret);
}

void i3c_imx3102_reg_write(struct i3c_dev_desc *imx3102, uint8_t addr, uint8_t *data, int data_size)
{
	uint8_t out[2];
	int ret;

	out[0] = addr;
	out[1] = 0;
	ret = i3c_jesd403_write(imx3102, out, 2, data, data_size);
	__ASSERT_NO_MSG(!ret);
}

void i3c_imx3102_release_ownership(struct i3c_dev_desc *imx3102)
{
	int ret;
	uint8_t select;

	if (imx3102->info.i2c_mode) {
		ret = i3c_i2c_read(imx3102, IMX3102_PORT_SEL, &select, 1);
		__ASSERT_NO_MSG(!ret);

		select ^= IMX3102_PORT_SEL_M1;
		ret = i3c_i2c_write(imx3102, IMX3102_PORT_SEL, &select, 1);
		__ASSERT_NO_MSG(!ret);
	} else {
		i3c_imx3102_reg_read(imx3102, IMX3102_PORT_SEL, &select, 1);
		select ^= IMX3102_PORT_SEL_M1;
		i3c_imx3102_reg_write(imx3102, 0x41, &select, 1);
	}
}

void i3c_imx3102_init(struct i3c_dev_desc *imx3102)
{
	uint8_t data[2];
	int ret;

	data[0] = IMX3102_PORT_CONF_M1_EN | IMX3102_PORT_CONF_S_EN;
	data[1] = IMX3102_PORT_SEL_S_EN;

	if (imx3102->info.i2c_mode) {
		ret = i3c_i2c_write(imx3102, IMX3102_PORT_CONF, data, 2);
		__ASSERT_NO_MSG(!ret);
	} else {
		i3c_imx3102_reg_write(imx3102, IMX3102_PORT_CONF, data, 2);
	}
}
