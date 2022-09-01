/*
 * Copyright (c) 2021 Aspeed Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <tc_util.h>
#include <kernel.h>
#include <device.h>
#include <drivers/i3c/i3c.h>
#include <random/rand32.h>
#include <soc.h>
#include "ast_test.h"

#define LOG_MODULE_NAME i3c_test

#include <logging/log.h>
LOG_MODULE_REGISTER(LOG_MODULE_NAME);


#define TEST_PRIV_XFER_SIZE	128
#define TEST_IBI_PAYLOAD_SIZE	128
#define MAX_DATA_SIZE		256

/* external reference */
int i3c_slave_mqueue_read(const struct device *dev, uint8_t *dest, int budget);
int i3c_slave_mqueue_write(const struct device *dev, uint8_t *src, int size);

#define TEST_I3C_SLAVE_THREAD_STACK_SIZE	512
#define TEST_I3C_SLAVE_THREAD_PRIO		CONFIG_ZTEST_THREAD_PRIORITY

K_THREAD_STACK_DEFINE(test_i3c_slave_thread_stack_area, TEST_I3C_SLAVE_THREAD_STACK_SIZE);
static struct k_thread test_i3c_slave_thread;

static uint8_t test_data_tx[MAX_DATA_SIZE];
static uint8_t test_data_rx[MAX_DATA_SIZE];
static struct i3c_ibi_payload i3c_payload;
static struct k_sem ibi_complete;
static struct k_sem data_ready;
static struct i3c_ibi_payload *test_ibi_write_requested(struct i3c_dev_desc *desc)
{
	i3c_payload.buf = test_data_rx;
	i3c_payload.size = 0;

	return &i3c_payload;
}

static void test_ibi_write_done(struct i3c_dev_desc *desc)
{
	k_sem_give(&ibi_complete);
}

static struct i3c_ibi_callbacks i3c_ibi_def_callbacks = {
	.write_requested = test_ibi_write_requested,
	.write_done = test_ibi_write_done,
};

static void prepare_test_data(uint8_t *data, int nbytes)
{
	uint32_t value;
	int i, shift;

	value = sys_rand32_get();
	for (i = 0; i < nbytes; i++) {
		shift = (i & 0x3) * 8;
		data[i] = (value >> shift) & 0xff;
		if ((i & 0x3) == 0x3) {
			value = sys_rand32_get();
		}
	}
}

static void test_i3c_slave_task(void *arg1, void *arg2, void *arg3)
{
	const struct device *slave_mq = (const struct device *)arg1;
	int ret;

	for (;;) {
		/* Test part 1: read and compare the data */
		while (i3c_slave_mqueue_read(slave_mq, (uint8_t *)test_data_rx,
					     TEST_PRIV_XFER_SIZE) == 0) {
			k_sleep(K_USEC(1));
		}
		ast_zassert_mem_equal(test_data_tx, test_data_rx, TEST_PRIV_XFER_SIZE,
				      "i3c private write test fail: data mismatch");

		/* Test part 2: send IBI to notify the master device to get the pending data */
		prepare_test_data(test_data_tx, TEST_IBI_PAYLOAD_SIZE);
		ret = i3c_slave_mqueue_write(slave_mq, test_data_tx, TEST_IBI_PAYLOAD_SIZE);
		k_sem_give(&data_ready);
		ast_zassert_equal(ret, 0, "failed to do slave mqueue write");
	}
}

static void test_i3c_ci(int count)
{
	const struct device *master, *slave_mq;
	struct i3c_dev_desc slave;
	struct i3c_priv_xfer xfer;
	int ret, i;

	master = device_get_binding(DT_LABEL(DT_NODELABEL(i3c0)));
	ast_zassert_not_null(master, "failed to get master device");

	slave_mq = device_get_binding(DT_LABEL(DT_NODELABEL(i3c1_smq)));
	ast_zassert_not_null(slave_mq, "failed to get slave mqueue device");

	k_sem_init(&ibi_complete, 0, 1);
	k_sem_init(&data_ready, 0, 1);

	/* create slave thread to service the slave actions */
	k_thread_create(&test_i3c_slave_thread,
			test_i3c_slave_thread_stack_area, TEST_I3C_SLAVE_THREAD_STACK_SIZE,
			test_i3c_slave_task, (void *)slave_mq, NULL, NULL,
			TEST_I3C_SLAVE_THREAD_PRIO, 0, K_NO_WAIT);

	slave.info.static_addr = DT_PROP(DT_BUS(DT_NODELABEL(i3c1_smq)), assigned_address);
	slave.info.assigned_dynamic_addr = slave.info.static_addr;
	slave.info.i2c_mode = 0;
	ret = i3c_master_attach_device(master, &slave);
	ast_zassert_equal(ret, 0, "failed to attach slave device onto the bus");

	ret = i3c_master_send_rstdaa(master);
	ret = i3c_master_send_rstdaa(master);
	ast_zassert_equal(ret, 0, "failed to send RSTDAA");

	ret = i3c_master_send_aasa(master);
	ast_zassert_equal(ret, 0, "failed to send SETAASA");

	ret = i3c_master_send_getpid(master, slave.info.dynamic_addr, &slave.info.pid);
	ast_zassert_equal(ret, 0, "failed to send GETPID");
	ast_zassert_equal(I3C_PID_VENDOR_ID(slave.info.pid), I3C_PID_VENDOR_ID_ASPEED,
			  "incorrect vendor ID %llx", slave.info.pid);

	ret = i3c_master_request_ibi(&slave, &i3c_ibi_def_callbacks);
	ast_zassert_equal(ret, 0, "failed to request sir");
	ret = i3c_master_enable_ibi(&slave);
	ast_zassert_equal(ret, 0, "failed to enable sir");

	for (i = 0; i < count; i++) {
		/*
		 * Test part 1:
		 * master --- private write transfer ---> slave
		 */
		prepare_test_data(test_data_tx, TEST_PRIV_XFER_SIZE);

		xfer.rnw = 0;
		xfer.len = TEST_PRIV_XFER_SIZE;
		xfer.data.out = test_data_tx;
		ret = i3c_master_priv_xfer(&slave, &xfer, 1);
		ast_zassert_equal(ret, 0, "failed to do private transfer");

		/*
		 * Test part 2:
		 * if MDB group ID is pending read notification:
		 *    master <--- IBI notification --------- slave
		 *    master ---- private read transfer ---> slave
		 * else:
		 *    master <--- IBI with data --------- slave
		 */

		/* master device waits for the IBI from the slave */
		k_sem_take(&ibi_complete, K_FOREVER);

		/* init the flag for the next loop */
		k_sem_init(&ibi_complete, 0, 1);

		/* check result: first byte (MDB) shall match the DT property mandatory-data-byte */
		ret = DT_PROP(DT_NODELABEL(i3c1_smq), mandatory_data_byte);
		ast_zassert_equal(ret, test_data_rx[0], "IBI MDB mismatch: %02x %02x\n", ret,
				  test_data_rx[0]);

		if (IS_MDB_PENDING_READ_NOTIFY(test_data_rx[0])) {
			/* to ensure the slave filled the pending read data */
			k_sem_take(&data_ready, K_FOREVER);
			k_sem_init(&data_ready, 0, 1);
			/* initiate a private read transfer to read the pending data */
			xfer.rnw = 1;
			xfer.len = TEST_IBI_PAYLOAD_SIZE;
			xfer.data.in = test_data_rx;
			ret = i3c_master_priv_xfer(&slave, &xfer, 1);
			ast_zassert_mem_equal(test_data_tx, test_data_rx, TEST_IBI_PAYLOAD_SIZE,
					      "data mismatch");
		} else {
			ast_zassert_mem_equal(test_data_tx, &test_data_rx[1], TEST_IBI_PAYLOAD_SIZE,
					      "data mismatch");
		}
	}

	ret = i3c_master_detach_device(master, &slave);
	ast_zassert_equal(ret, 0, "failed to detach device");

	k_thread_abort(&test_i3c_slave_thread);
}

int test_i3c(int count, enum aspeed_test_type type)
{
	printk("%s, count: %d, type: %d\n", __func__, count, type);

	if (type == AST_TEST_CI) {
		test_i3c_ci(count);
		return ast_ztest_result();
	}

	/* Not support FT yet */
	return AST_TEST_PASS;
}
