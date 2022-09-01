/*
 * Copyright (c) 2021 ASPEED Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr.h>
#include <sys/printk.h>
#include <string.h>
#include <device.h>
#include <drivers/ipmi/bt_aspeed.h>

struct bt_request {
	uint8_t len;
	uint8_t netfn;
	uint8_t seq;
	uint8_t cmd;
	uint8_t data[0];
};

struct bt_response {
	uint8_t len;
	uint8_t netfn;
	uint8_t seq;
	uint8_t cmd;
	uint8_t cmplt_code;
	uint8_t data[0];
};

void main(void)
{
	int i, rc;
	uint8_t ibuf[256];
	uint8_t obuf[256];
	const struct device *bt_dev;

	struct bt_request *req;
	struct bt_response *res;

	bt_dev = device_get_binding(DT_LABEL(DT_NODELABEL(bt)));
	if (!bt_dev) {
		printk("No BT device found\n");
		return;
	}

	while (1) {
		k_busy_wait(1000);

		rc = bt_aspeed_read(bt_dev, ibuf, sizeof(ibuf));
		if (rc < 0) {
			if (rc != -ENODATA)
				printk("failed to read BT data, rc=%d\n", rc);
			continue;
		}

		req = (struct bt_request *)ibuf;
		printk("BT req: length=0x%02x, netfn=0x%02x, seq=0x%02x, cmd=0x%02x, data:\n",
		       req->len, req->netfn, req->seq, req->cmd);
		for (i = 0; i < rc - 4; ++i) {
			if (i && (i % 16 == 0))
				printk("\n");
			printk("%02x ", req->data[i]);
		}
		printk("\n");

		res = (struct bt_response *)obuf;
		res->len = req->len + 1;
		res->netfn = req->netfn;
		res->seq = req->seq;
		res->cmd = req->cmd;
		res->cmplt_code = 0x0;
		memcpy(res->data, req->data, rc - 4);

		printk("BT res: length=0x%02x, netfn=0x%02x, seq=0x%02x, cmd=0x%02x, cmplt_code=0x%02x, data:\n",
		       res->len, res->netfn, res->seq, res->cmd, res->cmplt_code);
		for (i = 0; i < rc - 4; ++i) {
			if (i && (i % 16 == 0))
				printk("\n");
			printk("%02x ", res->data[i]);
		}
		printk("\n");

		rc = bt_aspeed_write(bt_dev, obuf, rc + 1);
		if (rc < 0) {
			printk("failed to write BT data, rc=%d\n", rc);
			continue;
		}
	}
}
