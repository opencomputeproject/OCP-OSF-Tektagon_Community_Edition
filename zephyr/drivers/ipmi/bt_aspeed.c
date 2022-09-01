/*
 * Copyright (c) 2021 ASPEED
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#define DT_DRV_COMPAT aspeed_bt

#include <device.h>
#include <kernel.h>
#include <soc.h>
#include <errno.h>
#include <string.h>
#include <logging/log.h>
#include <drivers/ipmi/bt_aspeed.h>
LOG_MODULE_REGISTER(bt_aspeed, CONFIG_LOG_DEFAULT_LEVEL);

/* LPC registers */
#define IBTCR0          0x140
#define   IBTCR0_ADDR_MASK      GENMASK(31, 18) /* [17:16] are RAZ */
#define   IBTCR0_ADDR_SHIFT     16
#define   IBTCR0_SIRQ_MASK      GENMASK(15, 12)
#define   IBTCR0_SIRQ_SHIFT     12
#define   IBTCR0_EN_CLR_SLV_RDP BIT(3)
#define   IBTCR0_EN_CLR_SLV_WRP BIT(2)
#define   IBTCR0_EN             BIT(0)
#define IBTCR1          0x144
#define   IBTCR1_INT_EN_HBUSY   BIT(6)
#define   IBTCR1_INT_EN_H2B     BIT(0)
#define IBTCR2          0x148
#define   IBTCR2_INT_HBUSY      BIT(6)
#define   IBTCR2_INT_H2B        BIT(0)
#define IBTCR3          0x14c
#define IBTCR4          0x150
#define IBTCR5          0x154
#define IBTCR6          0x158

/* IPMI 2.0 - BT Interface Registers */
#define BT_CTRL         IBTCR4
#define   BT_CTRL_B_BUSY        BIT(7)
#define   BT_CTRL_H_BUSY        BIT(6)
#define   BT_CTRL_OEM0          BIT(5)
#define   BT_CTRL_SMS_ATN       BIT(4)
#define   BT_CTRL_B2H_ATN       BIT(3)
#define   BT_CTRL_H2B_ATN       BIT(2)
#define   BT_CTRL_CLR_RDP       BIT(1)
#define   BT_CTRL_CLR_WRP       BIT(0)
#define BT_HOST2BMC     IBTCR5
#define BT_BMC2HOST     IBTCR5
#define BT_INTMASK      IBTCR6

/* misc. constant */
#define BT_FIFO_SIZE    0x100

static uintptr_t lpc_base;
#define LPC_RD(reg)             sys_read32(lpc_base + reg)
#define LPC_WR(val, reg)        sys_write32(val, lpc_base + reg)

struct bt_aspeed_config {
	uintptr_t base;
	uint32_t addr;
	uint32_t sirq;
};

/* IPMI 2.0 - BT_CTRL register bit R/W by BMC */
static inline void clr_b_busy(void)
{
	if (LPC_RD(BT_CTRL) & BT_CTRL_B_BUSY) {
		LPC_WR(BT_CTRL_B_BUSY, BT_CTRL);
	}
}

static inline void set_b_busy(void)
{
	if (!(LPC_RD(BT_CTRL) & BT_CTRL_B_BUSY)) {
		LPC_WR(BT_CTRL_B_BUSY, BT_CTRL);
	}
}

static inline void clr_oem0(void)
{
	LPC_WR(BT_CTRL_OEM0, BT_CTRL);
}

static inline void set_sms_atn(void)
{
	LPC_WR(BT_CTRL_SMS_ATN, BT_CTRL);
}

static inline void set_b2h_atn(void)
{
	LPC_WR(BT_CTRL_B2H_ATN, BT_CTRL);
}

static inline void clr_h2b_atn(void)
{
	LPC_WR(BT_CTRL_H2B_ATN, BT_CTRL);
}

static inline void clr_rd_ptr(void)
{
	LPC_WR(BT_CTRL_CLR_RDP, BT_CTRL);
}

static inline void clr_wr_ptr(void)
{
	LPC_WR(BT_CTRL_CLR_WRP, BT_CTRL);
}

static void bt_aspeed_isr(void)
{
	/*
	 * simply ack IRQ as currently
	 * only polling implementation
	 * is supported
	 */
	LPC_WR(LPC_RD(IBTCR2), IBTCR2);
}

int bt_aspeed_read(const struct device *dev,
		   uint8_t *buf, uint32_t buf_sz)
{
	int i, len;
	uint32_t reg;

	if (buf == NULL || !buf_sz) {
		return -EINVAL;
	}

	reg = LPC_RD(BT_CTRL);
	if (!(reg & BT_CTRL_H2B_ATN)) {
		return -ENODATA;
	}

	set_b_busy();
	clr_h2b_atn();
	clr_rd_ptr();

	buf[0] = LPC_RD(BT_HOST2BMC);
	len = (int)buf[0];
	len = ((len + 1) > buf_sz) ? buf_sz : len + 1;

	/* we pass the length back as well */
	for (i = 1; i < len; ++i)
		buf[i] = LPC_RD(BT_HOST2BMC);

	/* drop rest data */
	for (; i < buf[0]; ++i)
		reg = LPC_RD(BT_HOST2BMC);

	clr_b_busy();

	return len;
}

int bt_aspeed_write(const struct device *dev,
		    uint8_t *buf, uint32_t buf_sz)
{
	int i;
	uint32_t reg;

	if (buf == NULL || !buf_sz) {
		return -EINVAL;
	}

	/* Length + NetFn/LUN + Seq + Cmd + CmpltCode */
	if (buf_sz < 5 || buf_sz > BT_FIFO_SIZE) {
		return -EINVAL;
	}

	reg = LPC_RD(BT_CTRL);
	if (reg & (BT_CTRL_H_BUSY | BT_CTRL_B2H_ATN)) {
		return -EAGAIN;
	}

	clr_wr_ptr();

	for (i = 0; i < buf_sz; ++i)
		LPC_WR(buf[i], BT_BMC2HOST);

	set_b2h_atn();

	return i;
}

static int bt_aspeed_init(const struct device *dev)
{
	uint32_t reg;
	struct bt_aspeed_config *cfg = (struct bt_aspeed_config *)dev->config;

	if (!lpc_base) {
		lpc_base = cfg->base;
	}

	IRQ_CONNECT(DT_INST_IRQN(0),
		    DT_INST_IRQ(0, priority),
		    bt_aspeed_isr,
		    DEVICE_DT_INST_GET(0), 0);

	irq_enable(DT_INST_IRQN(0));

	reg = LPC_RD(IBTCR1);
	reg |= (IBTCR1_INT_EN_H2B | IBTCR1_INT_EN_HBUSY);
	LPC_WR(reg, IBTCR1);

	reg = ((cfg->addr << IBTCR0_ADDR_SHIFT) & IBTCR0_ADDR_MASK) |
	      ((cfg->sirq << IBTCR0_SIRQ_SHIFT) & IBTCR0_SIRQ_MASK) |
	      IBTCR0_EN_CLR_SLV_RDP |
	      IBTCR0_EN_CLR_SLV_WRP |
	      IBTCR0_EN;
	LPC_WR(reg, IBTCR0);

	clr_b_busy();

	return 0;
}

static struct bt_aspeed_config bt_aspeed_config = {
	.base = DT_REG_ADDR(DT_PARENT(DT_DRV_INST(0))),
	.addr = DT_INST_PROP(0, addr),
	.sirq = DT_INST_PROP(0, sirq),
};

DEVICE_DT_INST_DEFINE(0, bt_aspeed_init, NULL,
		      NULL, &bt_aspeed_config,
		      POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		      NULL);
