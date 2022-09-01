/*
 * Copyright (c) 2021 ASPEED Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr.h>
#include <sys/printk.h>
#include <string.h>
#include <device.h>
#include <drivers/espi_aspeed.h>

#define ESPI_PLD_LEN_MAX	(1UL << 12)

struct espi_oob_msg {
	uint8_t cyc;
	uint8_t len_h : 4;
	uint8_t tag : 4;
	uint8_t len_l;
	uint8_t data[];
};

static void hexdump(uint8_t *buf, uint32_t len)
{
	int i;

	for (i = 0; i < len; ++i) {
		if (i && (i % 16 == 0))
			printk("\n");
		printk("%02x ", buf[i]);
	}
	printk("\n");
}

void main(void)
{
	int rc;
	const struct device *espi_dev;
	struct espi_aspeed_ioc ioc;
	struct espi_oob_msg *oob_pkt;
	uint32_t oob_pkt_len;
	uint32_t cyc, tag, len;

	espi_dev = device_get_binding(DT_LABEL(DT_NODELABEL(espi)));
	if (!espi_dev) {
		printk("no eSPI device found\n");
		return;
	}

	oob_pkt_len = sizeof(*oob_pkt) + ESPI_PLD_LEN_MAX;
	oob_pkt = (struct espi_oob_msg *)k_malloc(oob_pkt_len);
	if (!oob_pkt) {
		printk("failed to allocate OOB packet\n");
		return;
	}

	while (1) {
		ioc.pkt_len = oob_pkt_len;
		ioc.pkt = (uint8_t *)oob_pkt;

		rc = espi_aspeed_oob_get_rx(espi_dev, &ioc, true);
		if (rc) {
			printk("failed to get OOB, rc=%d\n", rc);
			continue;
		}

		cyc = oob_pkt->cyc;
		tag = oob_pkt->tag;
		len = (oob_pkt->len_h << 8) | (oob_pkt->len_l & 0xff);

		printk("\n==== Receive RX Packet ====\n");
		printk("cyc=0x%02x, tag=0x%02x, len=0x%04x\n", cyc, tag, len);
		hexdump(oob_pkt->data, (len) ? len : ESPI_PLD_LEN_MAX);

		rc = espi_aspeed_oob_put_tx(espi_dev, &ioc);
		if (rc) {
			printk("failed to put OOB, rc=%d\n", rc);
			continue;
		}
	}

	k_free(oob_pkt);
}
