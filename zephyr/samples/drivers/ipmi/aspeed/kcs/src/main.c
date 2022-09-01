/*
 * Copyright (c) 2021 ASPEED Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <sys/printk.h>
#include <string.h>
#include <device.h>
#include <drivers/ipmi/kcs_aspeed.h>

struct kcs_request {
	uint8_t netfn;
	uint8_t cmd;
	uint8_t data[0];
};

struct kcs_response {
	uint8_t netfn;
	uint8_t cmd;
	uint8_t cmplt_code;
	uint8_t data[0];
};

void main(void)
{
	int i, rc;
	uint8_t ibuf[256];
	uint8_t obuf[256];
	const struct device *kcs_dev;

	struct kcs_request *req;
	struct kcs_response *res;

	kcs_dev = device_get_binding(DT_LABEL(DT_NODELABEL(kcs3)));
	if (!kcs_dev) {
		printk("No KCS device found\n");
		return;
	}

	while (1) {
		k_busy_wait(1000);

		rc = kcs_aspeed_read(kcs_dev, ibuf, sizeof(ibuf));
		if (rc < 0) {
			if (rc != -ENODATA)
				printk("failed to read KCS data, rc=%d\n", rc);
			continue;
		}

		req = (struct kcs_request *)ibuf;
		printk("KCS req: netfn=0x%02x, cmd=0x%02x, data:\n",
				req->netfn, req->cmd);
		for (i = 0; i < rc - 2; ++i) {
			if (i && (i % 16 == 0))
				printk("\n");
			printk("%02x ", req->data[i]);
		}
		printk("\n");

		res = (struct kcs_response *)obuf;
		res->netfn = req->netfn;
		res->cmd = req->cmd;
		res->cmplt_code = 0x0;
		memcpy(res->data, req->data, rc - 2);

		printk("KCS res: netfn=0x%02x, cmd=0x%02x, cmplt_code=0x%02x, data:\n",
				res->netfn, res->cmd, res->cmplt_code);
		for (i = 0; i < rc - 2; ++i) {
			if (i && (i % 16 == 0))
				printk("\n");
			printk("%02x ", res->data[i]);
		}
		printk("\n");

		rc = kcs_aspeed_write(kcs_dev, obuf, rc + 1);
		if (rc < 0) {
			printk("failed to write KCS data, rc=%d\n", rc);
			continue;
		}
	}
}
