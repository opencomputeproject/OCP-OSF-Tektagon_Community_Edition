/*
 * Copyright (c) 2021 ASPEED
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#define DT_DRV_COMPAT aspeed_snoop

#include <device.h>
#include <kernel.h>
#include <soc.h>
#include <errno.h>
#include <string.h>
#include <logging/log.h>
#include <drivers/clock_control.h>
#include <drivers/misc/aspeed/snoop_aspeed.h>
LOG_MODULE_REGISTER(snoop_aspeed, CONFIG_LOG_DEFAULT_LEVEL);

/* LPC registers */
#define	HICR5		0x080
#define	  HICR5_ENINT_SNP1W	BIT(3)
#define	  HICR5_EN_SNP1W	BIT(2)
#define	  HICR5_ENINT_SNP0W	BIT(1)
#define	  HICR5_EN_SNP0W	BIT(0)
#define	HICR6		0x084
#define	  HICR6_STR_SNP1W	BIT(1)
#define	  HICR6_STR_SNP0W	BIT(0)
#define	SNPWADR		0x090
#define	  SNPWADR_ADDR1_MASK	GENMASK(31, 16)
#define	  SNPWADR_ADDR1_SHIFT	16
#define	  SNPWADR_ADDR0_MASK	GENMASK(15, 0)
#define	  SNPWADR_ADDR0_SHIFT	0
#define	SNPWDR		0x094
#define	  SNPWDR_DATA1_MASK	GENMASK(15, 8)
#define	  SNPWDR_DATA1_SHIFT	8
#define	  SNPWDR_DATA0_MASK	GENMASK(7, 0)
#define	  SNPWDR_DATA0_SHIFT	0
#define	HICRB		0x100
#define	  HICRB_ENSNP1D		BIT(15)
#define	  HICRB_ENSNP0D		BIT(14)

/* misc. constant */
#define SNOOP_CHANNEL_NUM	2

static uintptr_t lpc_base;
#define LPC_RD(reg)             sys_read32(lpc_base + reg)
#define LPC_WR(val, reg)        sys_write32(val, lpc_base + reg)

struct snoop_aspeed_data {
	struct {
		uint8_t data;
		struct k_sem lock;
	} rx[SNOOP_CHANNEL_NUM];
};

struct snoop_aspeed_config {
	uintptr_t base;
	uint16_t port[SNOOP_CHANNEL_NUM];
};

int snoop_aspeed_read(const struct device *dev, uint32_t ch, uint8_t *out, bool blocking)
{
	int rc;
	struct snoop_aspeed_data *data = (struct snoop_aspeed_data *)dev->data;

	rc = k_sem_take(&data->rx[ch].lock, (blocking) ? K_FOREVER : K_NO_WAIT);
	if (rc)
		return rc;

	*out = data->rx[ch].data;

	return 0;
}

static void snoop_aspeed_isr(const struct device *dev)
{
	uint32_t hicr6, snpwdr;
	struct snoop_aspeed_data *data = (struct snoop_aspeed_data *)dev->data;

	hicr6 = LPC_RD(HICR6);
	snpwdr = LPC_RD(SNPWDR);

	if (hicr6 & HICR6_STR_SNP0W) {
		data->rx[0].data = (snpwdr & SNPWDR_DATA0_MASK) >> SNPWDR_DATA0_SHIFT;
		k_sem_give(&data->rx[0].lock);
	}

	if (hicr6 & HICR6_STR_SNP1W) {
		data->rx[1].data = (snpwdr & SNPWDR_DATA1_MASK) >> SNPWDR_DATA1_SHIFT;
		k_sem_give(&data->rx[1].lock);
	}

	LPC_WR(hicr6, HICR6);
}

static void snoop_aspeed_enable(const struct device *dev, uint32_t ch)
{
	uint32_t hicr5, snpwadr, hicrb;
	struct snoop_aspeed_config *cfg = (struct snoop_aspeed_config *)dev->config;

	hicr5 = LPC_RD(HICR5);
	snpwadr = LPC_RD(SNPWADR);
	hicrb = LPC_RD(HICRB);

	switch (ch) {
	case 0:
		hicr5 |= (HICR5_EN_SNP0W | HICR5_ENINT_SNP0W);
		snpwadr &= ~(SNPWADR_ADDR0_MASK);
		snpwadr |= ((cfg->port[0] << SNPWADR_ADDR0_SHIFT) & SNPWADR_ADDR0_MASK);
		hicrb |= HICRB_ENSNP0D;
		break;
	case 1:
		hicr5 |= (HICR5_EN_SNP1W | HICR5_ENINT_SNP1W);
		snpwadr &= ~(SNPWADR_ADDR1_MASK);
		snpwadr |= ((cfg->port[1] << SNPWADR_ADDR1_SHIFT) & SNPWADR_ADDR1_MASK);
		hicrb |= HICRB_ENSNP1D;
		break;
	default:
		LOG_ERR("unsupported snoop channel %d\n", ch);
		return;
	};

	LPC_WR(hicr5, HICR5);
	LPC_WR(snpwadr, SNPWADR);
	LPC_WR(hicrb, HICRB);
}

static int snoop_aspeed_init(const struct device *dev)
{
	int i;
	struct snoop_aspeed_data *data = (struct snoop_aspeed_data *)dev->data;
	struct snoop_aspeed_config *cfg = (struct snoop_aspeed_config *)dev->config;

	if (!lpc_base)
		lpc_base = cfg->base;

	IRQ_CONNECT(DT_INST_IRQN(0),
		    DT_INST_IRQ(0, priority),
		    snoop_aspeed_isr,
		    DEVICE_DT_INST_GET(0), 0);

	irq_enable(DT_INST_IRQN(0));

	for (i = 0; i < SNOOP_CHANNEL_NUM; ++i) {
		if (!cfg->port[i])
			continue;

		k_sem_init(&data->rx[i].lock, 0, 1);
		snoop_aspeed_enable(dev, i);
	}

	return 0;
}

static struct snoop_aspeed_data snoop_aspeed_data;

static struct snoop_aspeed_config snoop_aspeed_config = {
	.base = DT_REG_ADDR(DT_PARENT(DT_DRV_INST(0))),
	.port = { DT_INST_PROP_OR(0, port_IDX_0, 0),
		  DT_INST_PROP_OR(0, port_IDX_1, 0), },
};

DEVICE_DT_INST_DEFINE(0, snoop_aspeed_init, NULL,
		      &snoop_aspeed_data, &snoop_aspeed_config,
		      POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		      NULL);
