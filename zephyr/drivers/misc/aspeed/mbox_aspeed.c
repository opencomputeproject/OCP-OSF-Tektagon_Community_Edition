/*
 * Copyright (c) 2021 ASPEED
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#define DT_DRV_COMPAT aspeed_mbox

#include <device.h>
#include <kernel.h>
#include <soc.h>
#include <errno.h>
#include <string.h>
#include <logging/log.h>
#include <drivers/clock_control.h>
#include <drivers/misc/aspeed/mbox_aspeed.h>
LOG_MODULE_REGISTER(mbox_aspeed, CONFIG_LOG_DEFAULT_LEVEL);

/* Mailbox registers */
#define MBXDAT(n)	(0x200 + ((n) << 2))
#define MBXSTS(n)	(0x280 + ((n) << 2))
#define MBXBCR		0x290
#define   MBXBCR_RECV	BIT(7)
#define   MBXBCR_MASK	BIT(1)
#define   MBXBCR_SEND	BIT(0)
#define MBXBIE(n)	(0x2a0 + ((n) << 2))

static uintptr_t lpc_base;
#define LPC_RD(reg)             sys_read32(lpc_base + (reg))
#define LPC_WR(val, reg)        sys_write32(val, lpc_base + (reg))

struct mbox_aspeed_data {
	struct k_sem lock;
};

struct mbox_aspeed_config {
	uintptr_t base;
};

int mbox_aspeed_read(const struct device *dev, uint8_t *buf, size_t count, uint32_t off)
{
	int i, rc;
	uint32_t bcr;
	struct mbox_aspeed_data *data = (struct mbox_aspeed_data *)dev->data;

	if (!buf)
		return -EINVAL;

	if (off + count > MBX_DAT_REG_NUM)
		return -EINVAL;

	bcr = LPC_RD(MBXBCR);
	if (!(bcr & MBXBCR_RECV))
		return -ENODATA;

	rc = k_sem_take(&data->lock, K_NO_WAIT);
	if (rc) {
		return rc;
	}

	for (i = 0; i < count; ++i)
		buf[i] = (uint8_t)(LPC_RD(MBXDAT(i + off)) & 0xff);

	/* write 1 clear RECV and unmask as well */
	LPC_WR(MBXBCR_RECV, MBXBCR);

	k_sem_give(&data->lock);

	return 0;
}

int mbox_aspeed_write(const struct device *dev, uint8_t *buf, size_t count, uint32_t off)
{
	int i, rc;
	struct mbox_aspeed_data *data = (struct mbox_aspeed_data *)dev->data;

	if (!buf)
		return -EINVAL;

	if (off + count > MBX_DAT_REG_NUM)
		return -EINVAL;

	rc = k_sem_take(&data->lock, K_NO_WAIT);
	if (rc) {
		return rc;
	}

	for (i = 0; i < count ; ++i)
		LPC_WR(buf[i], MBXDAT(i + off));

	LPC_WR(MBXBCR_SEND, MBXBCR);

	k_sem_give(&data->lock);

	return 0;
}

static void mbox_aspeed_isr(const struct device *dev)
{
	uint32_t bcr;

	bcr = LPC_RD(MBXBCR);
	if (!(bcr & MBXBCR_RECV))
		return;

	/*
	 * leave the status bit set so that we know the data is for us,
	 * clear it once it has been read.
	 */

	/* mask it off, we'll clear it when the data gets read */
	LPC_WR(MBXBCR_MASK, MBXBCR);
}

static int mbox_aspeed_init(const struct device *dev)
{
	int i;
	struct mbox_aspeed_data *data = (struct mbox_aspeed_data *)dev->data;
	struct mbox_aspeed_config *cfg = (struct mbox_aspeed_config *)dev->config;

	if (!lpc_base)
		lpc_base = cfg->base;

	k_sem_init(&data->lock, 1, 1);

	/* disable & clear register-based interrupt */
	for (i = 0; i < MBX_DAT_REG_NUM / 8; ++i) {
		LPC_WR(0x0, MBXBIE(i));
		LPC_WR(0xf, MBXSTS(i));
	}

	IRQ_CONNECT(DT_INST_IRQN(0),
		    DT_INST_IRQ(0, priority),
		    mbox_aspeed_isr,
		    DEVICE_DT_INST_GET(0), 0);

	irq_enable(DT_INST_IRQN(0));

	return 0;
}

static struct mbox_aspeed_data mbox_aspeed_data;

static struct mbox_aspeed_config mbox_aspeed_config = {
	.base = DT_REG_ADDR(DT_PARENT(DT_DRV_INST(0))),
};

DEVICE_DT_INST_DEFINE(0, mbox_aspeed_init, NULL,
		      &mbox_aspeed_data, &mbox_aspeed_config,
		      POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		      NULL);
