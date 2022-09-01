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

/* external reference */
int i3c_slave_mqueue_read(const struct device *dev, uint8_t *dest, int budget);
int i3c_slave_mqueue_write(const struct device *dev, uint8_t *src, int size);

#ifdef CONFIG_I3C_SAMPLE_IMX3112
/*
 * IMX3112: 1-to-2 multiplexier
 *
 *                                        SPD@50  SPD@51
 * +----------------- +                   |       |
 * | SoC              |              /----+-------+---...
 * |                  |   +---------+   child bus 0, select IMX3112 local port 0
 * | I3C controller - | --| IMX3112 |
 * |                  |   +---------+   child bus 1, select IMX3112 local port 1
 * +------------------+              \----+-------+---...
 *                                        |       |
 *                                        SPD@52  SPD@53
 */
void i3c_imx3112_reg_read(struct i3c_dev_desc *imx3112, uint8_t addr, uint8_t *data, int data_size);
void i3c_imx3112_reg_write(struct i3c_dev_desc *imx3112, uint8_t addr, uint8_t *data,
			   int data_size);
void i3c_imx3112_select(struct i3c_dev_desc *imx3112, int chan);
void i3c_spd5118_reg_read(struct i3c_dev_desc *spd5118, uint8_t addr, uint8_t *data, int data_size);

static void i3c_imx3112_test(void)
{
	const struct device *master;
	struct i3c_dev_desc slave[5];
	int ret, i;
	uint8_t data[2];

	master = device_get_binding(DT_LABEL(DT_NODELABEL(i3c2)));
	__ASSERT(master, "master device not found\n");

	slave[0].info.static_addr = 0x70;
	slave[1].info.static_addr = 0x50;
	slave[2].info.static_addr = 0x51;
	slave[3].info.static_addr = 0x52;
	slave[4].info.static_addr = 0x53;

	for (i = 0; i < 5; i++) {
		slave[i].info.i2c_mode = 1;
		slave[i].info.assigned_dynamic_addr = slave[i].info.static_addr;
		ret = i3c_master_attach_device(master, &slave[i]);
		__ASSERT(!ret, "failed to attach i2c slave[%d]\n", i);
	}

	/* errata: must write 1 to MR46[0] before accessing */
	data[0] = 0x1;
	ret = i3c_i2c_write(&slave[0], 0x46, data, 1);
	__ASSERT_NO_MSG(!ret);

	ret = i3c_master_send_rstdaa(master);
	__ASSERT_NO_MSG(!ret);

	/* select child bus 0 */
	i3c_imx3112_select(&slave[0], 0);
	for (i = 0; i < 3; i++) {
		ret = i3c_i2c_read(&slave[i], 0, data, 2);
		__ASSERT_NO_MSG(!ret);
		printk("device%d ID in I2C mode %02x %02x\n", i, data[0], data[1]);
	}

	/* select child bus 1 */
	i3c_imx3112_select(&slave[0], 1);

	for (i = 3; i < 5; i++) {
		ret = i3c_i2c_read(&slave[i], 0, data, 2);
		__ASSERT_NO_MSG(!ret);
		printk("device%d ID in I2C mode %02x %02x\n", i, data[0], data[1]);
	}

	/* select child bus 0 */
	i3c_imx3112_select(&slave[0], 0);

	/* bring the bus to I3C mode */
	for (i = 0; i < 5; i++) {
		i3c_master_detach_device(master, &slave[i]);
		slave[i].info.i2c_mode = 0;
		ret = i3c_master_attach_device(master, &slave[i]);
		if (ret) {
			printk("failed to attach i3c slave[%d]\n", i);
			return;
		}
	}

	ret = i3c_master_send_aasa(master);
	__ASSERT(!ret, "SETAASA failed\n");

	for (i = 0; i < 3; i++) {
		i3c_spd5118_reg_read(&slave[i], 0, data, 2);
		printk("device%d ID in I3C mode %02x %02x\n", i, data[0], data[1]);
	}

	/* select child bus 1 */
	i3c_imx3112_select(&slave[0], 1);

	for (i = 3; i < 5; i++) {
		i3c_spd5118_reg_read(&slave[i], 0, data, 2);
		printk("device%d ID in I3C mode %02x %02x\n", i, data[0], data[1]);
	}
}
#endif

#ifdef CONFIG_I3C_SAMPLE_IMX3102
/*
 * IMX3102: 2-to-1 multiplexier
 *
 * +------------------   +
 * | SoC                 |
 * |                     |
 * | I3C controller #0 - | --+
 * |                     |    \                  SPD@51
 * |                     |     +---------+       |
 * |                     |     | IMX3102 | ---+--+---- i3c bus
 * |                     |     +---------+    |
 * |                     |    /               SPD@50
 * | I3C controller #1 - | --+
 * |                     |
 * +---------------------+
 */
void i3c_imx3102_reg_read(struct i3c_dev_desc *imx3102, uint8_t addr, uint8_t *data, int data_size);
void i3c_imx3102_reg_write(struct i3c_dev_desc *imx3102, uint8_t addr, uint8_t *data,
			   int data_size);
void i3c_imx3102_release_ownership(struct i3c_dev_desc *imx3102);
void i3c_imx3102_init(struct i3c_dev_desc *imx3102);
void i3c_spd5118_reg_read(struct i3c_dev_desc *spd5118, uint8_t addr, uint8_t *data, int data_size);

static void i3c_imx3102_test(void)
{
	const struct device *master[2];
	struct i3c_dev_desc slave[2][3];
	int ret, i, j;
	uint8_t data[2];

	master[0] = device_get_binding(DT_LABEL(DT_NODELABEL(i3c0)));
	__ASSERT(master[0], "master device not found\n");
	master[1] = device_get_binding(DT_LABEL(DT_NODELABEL(i3c1)));
	__ASSERT(master[1], "master device not found\n");

	/* init descriptors to the slave devices */
	for (i = 0; i < 2; i++) {
		slave[i][0].info.static_addr = 0xf;
		slave[i][1].info.static_addr = 0x50;
		slave[i][2].info.static_addr = 0x51;
		for (j = 0; j < 3; j++) {
			slave[i][j].info.i2c_mode = 1;
			slave[i][j].info.assigned_dynamic_addr = slave[i][j].info.static_addr;
			ret = i3c_master_attach_device(master[i], &slave[i][j]);
			__ASSERT(!ret, "failed to attach i2c slave%d to master%d\n", j, i);
		}
	}

	ret = i3c_master_send_rstdaa(master[0]);
	__ASSERT(!ret, "RSTDAA failed %d\n", ret);

	/* init the local port */
	i3c_imx3102_init(&slave[0][0]);

	for (j = 0; j < 3; j++) {
		ret = i3c_i2c_read(&slave[0][j], 0, data, 2);
		__ASSERT(!ret, "i2c xfer failed\n");
		printk("[I3C0][I2C mode] device%d ID %02x %02x\n", j, data[0], data[1]);
	}

	i3c_imx3102_release_ownership(&slave[0][0]);

	for (j = 0; j < 3; j++) {
		ret = i3c_i2c_read(&slave[1][j], 0, data, 2);
		__ASSERT(!ret, "i2c xfer failed\n");
		printk("[I3C1][I2C mode] device%d ID %02x %02x\n", j, data[0], data[1]);
	}

	for (i = 0; i < 2; i++) {
		for (j = 0; j < 3; j++) {
			i3c_master_detach_device(master[i], &slave[i][j]);
			slave[i][j].info.i2c_mode = 0;
			ret = i3c_master_attach_device(master[i], &slave[i][j]);
			__ASSERT(!ret, "failed to attach i3c slave%d to master%d\n", j, i);
		}
	}

	ret = i3c_master_send_aasa(master[1]);
	__ASSERT_NO_MSG(!ret);

	i3c_imx3102_reg_read(&slave[1][0], 0, data, 2);
	printk("[I3C1][I3C mode] device0 ID %02x %02x\n", data[0], data[1]);
	for (j = 1; j < 3; j++) {
		i3c_spd5118_reg_read(&slave[1][j], 0, data, 2);
		printk("[I3C1][I3C mode] device%d ID %02x %02x\n", j, data[0], data[1]);
	}

	i3c_imx3102_release_ownership(&slave[1][0]);

	i3c_imx3102_reg_read(&slave[0][0], 0, data, 2);
	printk("[I3C0][I3C mode] device0 ID %02x %02x\n", data[0], data[1]);
	for (j = 1; j < 3; j++) {
		i3c_spd5118_reg_read(&slave[0][j], 0, data, 2);
		printk("[I3C0][I3C mode] device%d ID %02x %02x\n", j, data[0], data[1]);
	}
}
#endif

#ifdef CONFIG_I3C_SAMPLE_LOOPBACK
static uint8_t ibi_data[CONFIG_I3C_ASPEED_MAX_IBI_PAYLOAD];
static struct i3c_ibi_payload i3c_payload;
static struct k_sem ibi_complete;
static struct i3c_ibi_payload *sample_ibi_write_requested(struct i3c_dev_desc *desc)
{
	i3c_payload.buf = ibi_data;
	i3c_payload.size = 0;

	return &i3c_payload;
}

static void sample_ibi_write_done(struct i3c_dev_desc *desc)
{
	int i;
	uint8_t *buf = (uint8_t *)i3c_payload.buf;

	for (i = 0; i < i3c_payload.size; i++) {
		if ((i & 0xf) == 0) {
			printk("\n");
		}
		printk("%02x ", buf[i]);
	}
	printk("\n");
	k_sem_give(&ibi_complete);
}

static struct i3c_ibi_callbacks i3c_ibi_def_callbacks = {
	.write_requested = sample_ibi_write_requested,
	.write_done = sample_ibi_write_done,
};

/**
 * @brief i3c0-to-i3c1 loopback test
 *
 * i3c0 plays the role as the main master and i3c1 plays the role as the slave device.
 * Use SETAASA to assign the dynamic address.
 *
 */
static void i3c_loopback_test(void)
{
	const struct device *master, *slave_mq;
	struct i3c_dev_desc slave;
	struct i3c_priv_xfer xfer;
	int ret, i;
	uint8_t data[16], result[16];

	master = device_get_binding(DT_LABEL(DT_NODELABEL(i3c0)));
	if (!master) {
		printk("master device not found\n");
		return;
	}

	slave_mq = device_get_binding(DT_LABEL(DT_NODELABEL(i3c1_smq)));
	if (!slave_mq) {
		printk("slave-mq device not found\n");
		return;
	}

	slave.info.static_addr = DT_PROP(DT_BUS(DT_NODELABEL(i3c1_smq)), assigned_address);
	slave.info.assigned_dynamic_addr = slave.info.static_addr;
	slave.info.i2c_mode = 0;
	ret = i3c_master_attach_device(master, &slave);
	if (ret) {
		printk("failed to attach slave\n");
		return;
	}

	/*
	 * ASPEED IOP fix:
	 * Aspeed slave device needs for a dummy CCC to activate the slave mode.
	 * The slave device will NACK the dummy CCC so just ignore the first response in the master
	 * side.
	 */
	ret = i3c_master_send_rstdaa(master);
	ret = i3c_master_send_rstdaa(master);
	if (ret) {
		printk("RSTDAA failed %d\n", ret);
		return;
	}

	ret = i3c_master_send_aasa(master);
	if (ret) {
		printk("SETAASA failed\n");
		return;
	}

	ret = i3c_master_send_getpid(master, slave.info.dynamic_addr, &slave.info.pid);
	printk("slave pid = %llx\n", slave.info.pid);

	printk("bus init done\n");

	for (i = 0; i < 16; i++) {
		data[i] = i;
	}

	xfer.rnw = 0;
	xfer.len = 4;
	for (i = 0; i < 16; i += 4) {
		xfer.data.out = &data[i];
		ret = i3c_master_priv_xfer(&slave, &xfer, 1);
		__ASSERT(!ret, "failed to private xfer at loop %d\n", i);

		ret = i3c_slave_mqueue_read(slave_mq, &result[i], 4);
		__ASSERT(ret == 4, "failed to read mqueue at loop %d\n", i);
	}

	for (i = 0; i < 16; i++) {
		__ASSERT(result[i] == data[i], "data mismatch: got %d, expected %d\n", result[i],
			 data[i]);
	}

	i3c_master_request_ibi(&slave, &i3c_ibi_def_callbacks);
	ret = i3c_master_enable_ibi(&slave);
	__ASSERT(ret == 0, "failed to enable sir: %d\n", ret);

	for (i = 0; i < 16; i++) {
		data[i] = 15 - i;
	}

	k_sem_init(&ibi_complete, 0, 1);
	ret = i3c_slave_mqueue_write(slave_mq, data, 16);
	k_sem_take(&ibi_complete, K_FOREVER);

	if (IS_MDB_PENDING_READ_NOTIFY(i3c_payload.buf[0])) {
		xfer.rnw = 1;
		xfer.len = 16;
		xfer.data.in = result;
		ret = i3c_master_priv_xfer(&slave, &xfer, 1);

		for (i = 0; i < 16; i++) {
			__ASSERT(result[i] == data[i], "data mismatch: got %d, expected %d\n",
				 result[i], data[i]);
		}
	} else {
		for (i = 0; i < 16; i++) {
			__ASSERT(i3c_payload.buf[i + 1] == data[i],
				 "data mismatch: got %d, expected %d\n", i3c_payload.buf[i + 1],
				 data[i]);
		}
	}

	printk("loopback test pass\n");
}
#endif

void main(void)
{
#ifdef CONFIG_I3C_SAMPLE_IMX3102
	i3c_imx3102_test();
#endif

#ifdef CONFIG_I3C_SAMPLE_IMX3112
	i3c_imx3112_test();
#endif

#ifdef CONFIG_I3C_SAMPLE_LOOPBACK
	i3c_loopback_test();
#endif
	while (1) {
		k_sleep(K_MSEC(1000));
	}
}
