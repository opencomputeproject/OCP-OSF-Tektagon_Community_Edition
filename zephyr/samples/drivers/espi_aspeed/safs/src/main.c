/*
 * Copyright (c) 2021 ASPEED Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr.h>
#include <sys/printk.h>
#include <sys/byteorder.h>
#include <string.h>
#include <device.h>
#include <drivers/espi_aspeed.h>

#define ESPI_PLD_LEN_MAX	(1UL << 12)
#define ESPI_PLD_LEN_MIN	(1UL << 6)

#define ESPI_FLASH_READ			0x00
#define ESPI_FLASH_WRITE		0x01
#define ESPI_FLASH_ERASE		0x02
#define ESPI_FLASH_SUC_CMPLT		0x06
#define ESPI_FLASH_SUC_CMPLT_D_MIDDLE	0x09
#define ESPI_FLASH_SUC_CMPLT_D_FIRST	0x0b
#define ESPI_FLASH_SUC_CMPLT_D_LAST	0x0d
#define ESPI_FLASH_SUC_CMPLT_D_ONLY	0x0f
#define ESPI_FLASH_UNSUC_CMPLT		0x0c

struct espi_flash_rwe {
	uint8_t cyc;
	uint8_t len_h : 4;
	uint8_t tag : 4;
	uint8_t len_l;
	uint32_t addr_be;
	uint8_t data[];
} __packed;

struct espi_flash_cmplt {
	uint8_t cyc;
	uint8_t len_h : 4;
	uint8_t tag : 4;
	uint8_t len_l;
	uint8_t data[];
} __packed;

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

static int espi_safs_read(const struct device *dev, uint8_t tag, uint32_t addr, uint32_t len)
{
	int i, rc;
	uint32_t cnt = 0;
	struct espi_aspeed_ioc ioc;
	struct espi_flash_cmplt *cmplt_pkt;
	uint32_t cmplt_pkt_len;

	cmplt_pkt_len = sizeof(*cmplt_pkt) + ESPI_PLD_LEN_MAX;
	cmplt_pkt = (struct espi_flash_cmplt *)k_malloc(cmplt_pkt_len);
	if (!cmplt_pkt) {
		printk("cannot allocate completion packet\n");
		return -ENOMEM;
	}

	if (len <= ESPI_PLD_LEN_MIN) {
		cmplt_pkt->cyc = ESPI_FLASH_SUC_CMPLT_D_ONLY;
		cmplt_pkt->tag = tag;
		cmplt_pkt->len_h = len >> 8;
		cmplt_pkt->len_l = len & 0xff;
		for (i = 0; i < len; ++i)
			cmplt_pkt->data[i] = i;

		ioc.pkt_len = len + sizeof(*cmplt_pkt);
		ioc.pkt = (uint8_t *)cmplt_pkt;

		rc = espi_aspeed_flash_put_tx(dev, &ioc);
		if (rc) {
			printk("failed to put the only flash completion, rc=%d\n", rc);
			goto free_n_out;
		}
	} else {
		/* first data */
		cmplt_pkt->cyc = ESPI_FLASH_SUC_CMPLT_D_FIRST;
		cmplt_pkt->tag = tag;
		cmplt_pkt->len_h = ESPI_PLD_LEN_MIN >> 8;
		cmplt_pkt->len_l = ESPI_PLD_LEN_MIN & 0xff;
		for (i = 0; i < len; ++i)
			cmplt_pkt->data[i] = cnt++;

		ioc.pkt_len = ESPI_PLD_LEN_MIN + sizeof(*cmplt_pkt);
		ioc.pkt = (uint8_t *)cmplt_pkt;

		rc = espi_aspeed_flash_put_tx(dev, &ioc);
		if (rc) {
			printk("failed to put the first flash completion, rc=%d\n", rc);
			goto free_n_out;
		}

		len -= ESPI_PLD_LEN_MIN;
		cnt += ESPI_PLD_LEN_MIN;

		/* middle data */
		while (len > ESPI_PLD_LEN_MIN) {
			cmplt_pkt->cyc = ESPI_FLASH_SUC_CMPLT_D_MIDDLE;
			cmplt_pkt->tag = tag;
			cmplt_pkt->len_h = ESPI_PLD_LEN_MIN >> 8;
			cmplt_pkt->len_l = ESPI_PLD_LEN_MIN & 0xff;
			for (i = 0; i < ESPI_PLD_LEN_MIN; ++i)
				cmplt_pkt->data[i] = cnt++;

			ioc.pkt_len = ESPI_PLD_LEN_MIN + sizeof(*cmplt_pkt);
			ioc.pkt = (uint8_t *)cmplt_pkt;

			rc = espi_aspeed_flash_put_tx(dev, &ioc);
			if (rc) {
				printk("failed to put the middle flash completion, rc=%d\n", rc);
				goto free_n_out;
			}

			len -= ESPI_PLD_LEN_MIN;
			cnt += ESPI_PLD_LEN_MIN;
		}

		/* last data */
		cmplt_pkt->cyc = ESPI_FLASH_SUC_CMPLT_D_LAST;
		cmplt_pkt->tag = tag;
		cmplt_pkt->len_h = len >> 8;
		cmplt_pkt->len_l = len & 0xff;
		for (i = 0; i < len; ++i)
			cmplt_pkt->data[i] = cnt++;

		ioc.pkt_len = len + sizeof(*cmplt_pkt);
		ioc.pkt = (uint8_t *)cmplt_pkt;

		rc = espi_aspeed_flash_put_tx(dev, &ioc);
		if (rc) {
			printk("failed to put thelast flash completion, rc=%d\n", rc);
			goto free_n_out;
		}
	}

free_n_out:
	k_free(cmplt_pkt);

	return rc;
}

static int espi_safs_write(const struct device *dev, uint8_t tag, uint32_t addr, uint32_t len,
			   uint8_t *buf)
{
	int rc;
	struct espi_aspeed_ioc ioc;
	struct espi_flash_cmplt cmplt_pkt;

	cmplt_pkt.cyc = ESPI_FLASH_SUC_CMPLT;
	cmplt_pkt.tag = tag;
	cmplt_pkt.len_h = 0;
	cmplt_pkt.len_l = 0;

	ioc.pkt_len = sizeof(cmplt_pkt);
	ioc.pkt = (uint8_t *)&cmplt_pkt;

	rc = espi_aspeed_flash_put_tx(dev, &ioc);
	if (rc) {
		printk("failed to put flash completion, rc=%d\n", rc);
		return rc;
	}

	return 0;
}

static int espi_safs_erase(const struct device *dev, uint8_t tag, uint32_t addr, uint32_t len)
{
	int rc;
	struct espi_aspeed_ioc ioc;
	struct espi_flash_cmplt cmplt_pkt;

	cmplt_pkt.cyc = ESPI_FLASH_SUC_CMPLT;
	cmplt_pkt.tag = tag;
	cmplt_pkt.len_h = 0;
	cmplt_pkt.len_l = 0;

	ioc.pkt_len = sizeof(cmplt_pkt);
	ioc.pkt = (uint8_t *)&cmplt_pkt;

	rc = espi_aspeed_flash_put_tx(dev, &ioc);
	if (rc) {
		printk("failed to put flash completion, rc=%d\n", rc);
		return rc;
	}

	return 0;
}

void main(void)
{
	int rc;
	const struct device *espi_dev;
	struct espi_aspeed_ioc ioc;
	struct espi_flash_rwe *rwe_pkt;
	uint32_t rwe_pkt_len;
	uint32_t cyc, tag, len, addr;
	uint32_t reg;

	espi_dev = device_get_binding(DT_LABEL(DT_NODELABEL(espi)));
	if (!espi_dev) {
		printk("no eSPI device found\n");
		return;
	}

	rwe_pkt_len = sizeof(*rwe_pkt) + ESPI_PLD_LEN_MAX;
	rwe_pkt = (struct espi_flash_rwe *)k_malloc(rwe_pkt_len);
	if (!rwe_pkt) {
		printk("failed to allocate flash packet\n");
		return;
	}

	/* enable SAFS */
	reg = *((volatile uint32_t *)(0x7e6e2510));
	reg |= BIT(7);
	*((volatile uint32_t *)(0x7e6e2510)) = reg;

	while (1) {
		ioc.pkt_len = rwe_pkt_len;
		ioc.pkt = (uint8_t *)rwe_pkt;

		rc = espi_aspeed_flash_get_rx(espi_dev, &ioc, true);
		if (rc) {
			printk("failed to get flash packet, rc=%d\n", rc);
			continue;
		}

		cyc = rwe_pkt->cyc;
		tag = rwe_pkt->tag;
		len = (rwe_pkt->len_h << 8) | (rwe_pkt->len_l & 0xff);
		addr = __bswap_32(rwe_pkt->addr_be);

		printk("\n==== Receive RX Packet ====\n");
		printk("cyc=0x%02x, tag=0x%02x, len=0x%04x, addr=0x%08x\n", cyc, tag, len, addr);
		if (cyc == ESPI_FLASH_WRITE)
			hexdump(rwe_pkt->data, (len) ? len : ESPI_PLD_LEN_MAX);

		switch (cyc) {
		case ESPI_FLASH_READ:
			rc = espi_safs_read(espi_dev, tag, addr, len);
			break;
		case ESPI_FLASH_WRITE:
			rc = espi_safs_write(espi_dev, tag, addr, len, rwe_pkt->data);
			break;
		case ESPI_FLASH_ERASE:
			rc = espi_safs_erase(espi_dev, tag, addr, len);
			break;
		default:
			rc = -EFAULT;
			break;
		}

		if (rc) {
			printk("failed to put flash packet, rc=%d\n", rc);
			continue;
		}
	}

	k_free(rwe_pkt);
}
