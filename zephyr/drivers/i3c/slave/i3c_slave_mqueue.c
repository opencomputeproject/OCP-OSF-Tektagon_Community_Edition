/*
 * Copyright (c) 2021 ASPEED Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#define DT_DRV_COMPAT aspeed_i3c_slave_mqueue

#include <soc.h>
#include <sys/util.h>
#include <device.h>
#include <kernel.h>
#include <init.h>
#include <drivers/i3c/i3c.h>
#include <sys/sys_io.h>
#define LOG_LEVEL CONFIG_I3C_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(i3c_slave_mqueue);

struct i3c_slave_mqueue_config {
	char *controller_name;
	int msg_size;
	int num_of_msgs;
	int mdb;
};

struct i3c_slave_mqueue_obj {
	const struct device *i3c_controller;
	struct i3c_slave_payload *msg_curr;
	struct i3c_slave_payload *msg_queue;
	int in;
	int out;
};

#define DEV_CFG(dev)			((struct i3c_slave_mqueue_config *)(dev)->config)
#define DEV_DATA(dev)			((struct i3c_slave_mqueue_obj *)(dev)->data)

static struct i3c_slave_payload *i3c_slave_mqueue_write_requested(const struct device *dev)
{
	struct i3c_slave_mqueue_obj *obj = DEV_DATA(dev);

	return obj->msg_curr;
}

static void i3c_slave_mqueue_write_done(const struct device *dev)
{
	struct i3c_slave_mqueue_obj *obj = DEV_DATA(dev);
	struct i3c_slave_mqueue_config *config = DEV_CFG(dev);

#ifdef DBG_DUMP
	int i;
	uint8_t *buf = (uint8_t *)obj->msg_curr->buf;

	printk("%s: %d %d\n", __func__, obj->in, obj->out);
	for (i = 0; i < obj->msg_curr->size; i++) {
		printk("%02x\n", buf[i]);
	}
#endif

	/* update pointer */
	obj->in = (obj->in + 1) & (config->num_of_msgs - 1);
	obj->msg_curr = &obj->msg_queue[obj->in];

	/* if queue full, skip the oldest un-read message */
	if (obj->in == obj->out) {
		LOG_WRN("buffer overflow\n");
		obj->out = (obj->out + 1) & (config->num_of_msgs - 1);
	}
}

static const struct i3c_slave_callbacks i3c_slave_mqueue_callbacks = {
	.write_requested = i3c_slave_mqueue_write_requested,
	.write_done = i3c_slave_mqueue_write_done,
};

/**
 * @brief application reads the data from the message queue
 *
 * @param dev i3c slave mqueue device
 * @return int -1: message queue empty
 */
int i3c_slave_mqueue_read(const struct device *dev, uint8_t *dest, int budget)
{
	struct i3c_slave_mqueue_config *config = DEV_CFG(dev);
	struct i3c_slave_mqueue_obj *obj = DEV_DATA(dev);
	struct i3c_slave_payload *msg;
	int ret;

	if (obj->out == obj->in) {
		return 0;
	}

	msg = &obj->msg_queue[obj->out];
	ret = (msg->size > budget) ? budget : msg->size;
	memcpy(dest, msg->buf, ret);

	obj->out = (obj->out + 1) & (config->num_of_msgs - 1);

	return ret;
}


int i3c_slave_mqueue_write(const struct device *dev, uint8_t *src, int size)
{
	struct i3c_slave_mqueue_config *config = DEV_CFG(dev);
	struct i3c_slave_mqueue_obj *obj = DEV_DATA(dev);

	if (IS_MDB_PENDING_READ_NOTIFY(config->mdb)) {
		i3c_slave_send_sir(obj->i3c_controller, config->mdb, NULL, 0);
		return i3c_slave_prep_read_data(obj->i3c_controller, src, size, false);
	} else {
		return i3c_slave_send_sir(obj->i3c_controller, config->mdb, src, size);
	}
}

static void i3c_slave_mqueue_init(const struct device *dev)
{
	struct i3c_slave_mqueue_config *config = DEV_CFG(dev);
	struct i3c_slave_mqueue_obj *obj = DEV_DATA(dev);
	struct i3c_slave_setup slave_data;
	uint8_t *buf;
	int i;

	LOG_DBG("msg size %d, n %d\n", config->msg_size, config->num_of_msgs);
	LOG_DBG("bus name : %s\n", config->controller_name);
	__ASSERT((config->num_of_msgs & (config->num_of_msgs - 1)) == 0,
		 "number of msgs must be power of 2\n");

	obj->i3c_controller = device_get_binding(config->controller_name);

	buf = k_calloc(config->msg_size, config->num_of_msgs);
	if (!buf) {
		LOG_ERR("failed to create message buffer\n");
		return;
	}

	obj->msg_queue = (struct i3c_slave_payload *)k_malloc(sizeof(struct i3c_slave_payload) *
							      config->num_of_msgs);
	if (!obj->msg_queue) {
		LOG_ERR("failed to create message queue\n");
		return;
	}

	for (i = 0; i < config->num_of_msgs; i++) {
		obj->msg_queue[i].buf = buf + (i * config->msg_size);
		obj->msg_queue[i].size = 0;
	}

	obj->in = 0;
	obj->out = 0;
	obj->msg_curr = &obj->msg_queue[0];

	slave_data.max_payload_len = config->msg_size;
	slave_data.callbacks = &i3c_slave_mqueue_callbacks;
	slave_data.dev = dev;
	i3c_slave_register(obj->i3c_controller, &slave_data);
}

#define I3C_SLAVE_MQUEUE_INIT(n)                                                                   \
	static int i3c_slave_mqueue_config_func_##n(const struct device *dev);                     \
	static const struct i3c_slave_mqueue_config i3c_slave_mqueue_config_##n = {                \
		.controller_name = DT_INST_BUS_LABEL(n),                                           \
		.msg_size = DT_INST_PROP(n, msg_size),                                             \
		.num_of_msgs = DT_INST_PROP(n, num_of_msgs),                                       \
		.mdb = DT_INST_PROP(n, mandatory_data_byte),                                       \
	};                                                                                         \
												   \
	static struct i3c_slave_mqueue_obj i3c_slave_mqueue_obj##n;                                \
												   \
	DEVICE_DT_INST_DEFINE(n, &i3c_slave_mqueue_config_func_##n, NULL,                          \
			      &i3c_slave_mqueue_obj##n, &i3c_slave_mqueue_config_##n, POST_KERNEL, \
			      CONFIG_I3C_SLAVE_INIT_PRIORITY, NULL);                               \
												   \
	static int i3c_slave_mqueue_config_func_##n(const struct device *dev)                      \
	{                                                                                          \
		i3c_slave_mqueue_init(dev);                                                        \
		return 0;                                                                          \
	}

DT_INST_FOREACH_STATUS_OKAY(I3C_SLAVE_MQUEUE_INIT)
