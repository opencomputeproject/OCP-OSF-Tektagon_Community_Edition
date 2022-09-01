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

int i3c_master_send_enec(const struct device *master, uint8_t addr, uint8_t evt)
{
	struct i3c_ccc_cmd ccc;
	uint8_t event = evt;

	ccc.addr = addr;
	ccc.id = I3C_CCC_ENEC;
	if (addr != I3C_BROADCAST_ADDR) {
		ccc.id |= I3C_CCC_DIRECT;
	}
	ccc.payload.length = 1;
	ccc.payload.data = &event;
	ccc.rnw = 0;
	ccc.ret = 0;

	return i3c_master_send_ccc(master, &ccc);
}

int i3c_master_send_rstdaa(const struct device *master)
{
	struct i3c_ccc_cmd ccc;

	/* RSTDAA CCC */
	ccc.addr = I3C_BROADCAST_ADDR;
	ccc.id = I3C_CCC_RSTDAA;
	ccc.payload.length = 0;
	ccc.payload.data = NULL;
	ccc.rnw = 0;
	ccc.ret = 0;

	return i3c_master_send_ccc(master, &ccc);
}

int i3c_master_send_aasa(const struct device *master)
{
	struct i3c_ccc_cmd ccc;

	ccc.addr = I3C_BROADCAST_ADDR;
	ccc.id = I3C_CCC_SETAASA;
	ccc.payload.length = 0;
	ccc.payload.data = NULL;
	ccc.rnw = 0;
	ccc.ret = 0;

	return i3c_master_send_ccc(master, &ccc);
}

int i3c_master_send_setmrl(const struct device *master, uint8_t addr, uint16_t mrl,
			   uint8_t ibi_payload_size)
{
	struct i3c_ccc_cmd ccc;
	uint8_t payload[4];

	payload[0] = mrl >> 8;
	payload[1] = mrl & 0xff;
	payload[2] = ibi_payload_size;

	ccc.addr = addr;
	ccc.id = I3C_CCC_SETMRL;
	if (addr != I3C_BROADCAST_ADDR) {
		ccc.id |= I3C_CCC_DIRECT;
	}
	ccc.payload.length = ibi_payload_size ? 3 : 2;
	ccc.payload.data = payload;
	ccc.rnw = 0;
	ccc.ret = 0;

	return i3c_master_send_ccc(master, &ccc);
}

int i3c_master_send_getpid(const struct device *master, uint8_t addr, uint64_t *pid)
{
	struct i3c_ccc_cmd ccc;
	int ret;
	uint8_t payload[8];

	ccc.addr = addr;
	ccc.payload.length = 6;
	ccc.payload.data = payload;

	ccc.rnw = 1;
	ccc.id = I3C_CCC_GETPID;
	ccc.ret = 0;

	ret = i3c_master_send_ccc(master, &ccc);
	if (ret) {
		return ret;
	}

	*pid = 0;
	for (int i = 0; i < 6; i++) {
		int sft = (6 - i - 1) * 8;
		*pid |= (uint64_t)payload[i] << sft;
	}

	return 0;
}

/**
 * @brief data read for the JESD compliant devices
 * @param slave the JESD compliant device
 * @param addr the address buffer
 * @param addr_size size of the address buffer in byte
 * @param data buffer to store the read data
 * @param data_size size of the read data in byte
 * @return 0 if success
 */
int i3c_jesd403_read(struct i3c_dev_desc *slave, uint8_t *addr, int addr_size, uint8_t *data,
		     int data_size)
{
	struct i3c_priv_xfer xfer[2];

	__ASSERT(slave->master_dev, "Unregistered device\n");
	__ASSERT(!slave->info.i2c_mode, "Not I3C device\n\n");

	xfer[0].rnw = 0;
	xfer[0].len = addr_size;
	xfer[0].data.out = addr;

	xfer[1].rnw = 1;
	xfer[1].len = data_size;
	xfer[1].data.in = data;

	return i3c_master_priv_xfer(slave, xfer, 2);
}

/**
 * @brief data write for the JESD compliant devices
 * @param slave the JESD compliant device
 * @param addr the address buffer
 * @param addr_size size of the address buffer in byte
 * @param data buffer to store the write data
 * @param data_size size of the write data in byte
 * @return 0 if success
 */
int i3c_jesd403_write(struct i3c_dev_desc *slave, uint8_t *addr, int addr_size, uint8_t *data,
		      int data_size)
{
	struct i3c_priv_xfer xfer[1];
	uint8_t *out;
	int ret;

	__ASSERT(slave->master_dev, "Unregistered device\n");
	__ASSERT(!slave->info.i2c_mode, "Not I3C device\n\n");

	out = k_calloc(sizeof(uint8_t), addr_size + data_size);
	memcpy(&out[0], addr, addr_size);
	memcpy(&out[addr_size], data, data_size);

	xfer[0].rnw = 0;
	xfer[0].len = addr_size + data_size;
	xfer[0].data.out = out;

	ret = i3c_master_priv_xfer(slave, xfer, 1);
	k_free(out);

	return ret;
}

/**
 * @brief data read for the I2C devices
 * @param slave the I2C device
 * @param add the address to be read
 * @param buf buffer to store the read data
 * @param length length of the read data
 * @return 0 if success
 */
int i3c_i2c_read(struct i3c_dev_desc *slave, uint8_t addr, uint8_t *buf, int length)
{
	struct i3c_priv_xfer xfer;
	uint8_t mode_reg = addr;
	int ret;

	__ASSERT(slave->master_dev, "Unregistered device\n");
	__ASSERT(slave->info.i2c_mode, "Not I2C device\n\n");

	xfer.rnw = 0;
	xfer.len = 1;
	xfer.data.out = &mode_reg;
	ret = i3c_master_priv_xfer(slave, &xfer, 1);
	if (ret) {
		return ret;
	}

	xfer.rnw = 1;
	xfer.len = length;
	xfer.data.in = buf;
	return i3c_master_priv_xfer(slave, &xfer, 1);
}

/**
 * @brief data write for the I2C devices
 * @param slave the I2C device
 * @param add the address to be write
 * @param buf buffer to store the write data
 * @param length length of the write data
 * @return 0 if success
 */
int i3c_i2c_write(struct i3c_dev_desc *slave, uint8_t addr, uint8_t *buf, int length)
{
	struct i3c_priv_xfer xfer;
	uint8_t *out;
	int ret;

	__ASSERT(slave->master_dev, "Unregistered device\n");
	__ASSERT(slave->info.i2c_mode, "Not I2C device\n\n");

	out = k_calloc(sizeof(uint8_t), length + 1);
	out[0] = addr;
	memcpy(&out[1], buf, length);

	xfer.rnw = 0;
	xfer.len = length + 1;
	xfer.data.out = out;
	ret = i3c_master_priv_xfer(slave, &xfer, 1);
	k_free(out);

	return ret;
}
