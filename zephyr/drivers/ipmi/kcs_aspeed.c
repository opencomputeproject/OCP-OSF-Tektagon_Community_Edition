/*
 * Copyright (c) 2021 ASPEED
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#define DT_DRV_COMPAT aspeed_kcs

#include <device.h>
#include <kernel.h>
#include <soc.h>
#include <errno.h>
#include <string.h>
#include <logging/log.h>
#include <drivers/clock_control.h>
#include <drivers/ipmi/kcs_aspeed.h>
LOG_MODULE_REGISTER(kcs_aspeed, CONFIG_LOG_DEFAULT_LEVEL);

/* LPC registers */
#define HICR0           0x000
#define   HICR0_LPC3E           BIT(7)
#define   HICR0_LPC2E           BIT(6)
#define   HICR0_LPC1E           BIT(5)
#define HICR2           0x008
#define   HICR2_IBFIF3          BIT(3)
#define   HICR2_IBFIF2          BIT(2)
#define   HICR2_IBFIF1          BIT(1)
#define HICR4           0x010
#define   HICR4_LADR12AS        BIT(7)
#define   HICR4_KCSENBL         BIT(2)
#define LADR3H          0x014
#define LADR3L          0x018
#define LADR12H         0x01c
#define LADR12L         0x020
#define IDR1            0x024
#define IDR2            0x028
#define IDR3            0x02c
#define ODR1            0x030
#define ODR2            0x034
#define ODR3            0x038
#define STR1            0x03c
#define STR2            0x040
#define STR3            0x044
#define HICRB           0x100
#define   HICRB_ENSNP1D         BIT(15)
#define   HICRB_ENSNP0D         BIT(14)
#define   HICRB_IBFIF4          BIT(1)
#define   HICRB_LPC4E           BIT(0)
#define LADR4           0x110
#define IDR4            0x114
#define ODR4            0x118
#define STR4            0x11C
#define  STR4_STAT_MASK         GENMASK(7, 6)
#define  STR4_STAT_SHIFT        6

/* misc. constant */
#define KCS_DUMMY_ZERO  0x0
#define KCS_BUF_SIZE    0x100

static uintptr_t lpc_base;
#define LPC_RD(reg)             sys_read32(lpc_base + reg)
#define LPC_WR(val, reg)        sys_write32(val, lpc_base + reg)

enum kcs_aspeed_chan {
	KCS_CH1 = 1,
	KCS_CH2,
	KCS_CH3,
	KCS_CH4,
};

struct kcs_aspeed_data {
	uint32_t idr;
	uint32_t odr;
	uint32_t str;

	uint8_t ibuf[KCS_BUF_SIZE];
	uint32_t ibuf_idx;
	uint32_t ibuf_avail;

	uint8_t obuf[KCS_BUF_SIZE];
	uint32_t obuf_idx;
	uint32_t obuf_data_sz;

	uint32_t phase;
	uint32_t error;
};

struct kcs_aspeed_config {
	uintptr_t base;
	uint32_t chan;
	uint32_t addr;
	void (*irq_config_func)(const struct device *dev);
};

/* IPMI 2.0 - Table 9-1, KCS Interface Status Register Bits */
#define KCS_STR_STATE_MASK      GENMASK(7, 6)
#define KCS_STR_STATE_SHIFT     6
#define KCS_STR_CMD_DAT         BIT(3)
#define KCS_STR_SMS_ATN         BIT(2)
#define KCS_STR_IBF             BIT(1)
#define KCS_STR_OBF             BIT(0)

/* IPMI 2.0 - Table 9-2, KCS Interface State Bits */
enum kcs_state {
	KCS_STATE_IDLE,
	KCS_STATE_READ,
	KCS_STATE_WRITE,
	KCS_STATE_ERROR,
	KCS_STATE_NUM
};

/* IPMI 2.0 - Table 9-3, KCS Interface Control Codes */
enum kcs_cmd_code {
	KCS_CMD_GET_STATUS_ABORT        = 0x60,
	KCS_CMD_WRITE_START             = 0x61,
	KCS_CMD_WRITE_END               = 0x62,
	KCS_CMD_READ_BYTE               = 0x68,
	KCS_CMD_NUM
};

/* IPMI 2.0 - Table 9-4, KCS Interface Status Codes */
enum kcs_error_code {
	KCS_NO_ERROR                    = 0x00,
	KCS_ABORTED_BY_COMMAND          = 0x01,
	KCS_ILLEGAL_CONTROL_CODE        = 0x02,
	KCS_LENGTH_ERROR                = 0x06,
	KCS_UNSPECIFIED_ERROR           = 0xff
};

/* IPMI 2.0 - Figure 9. KCS Phase in Transfer Flow Chart */
enum kcs_phase {
	KCS_PHASE_IDLE,

	KCS_PHASE_WRITE_START,
	KCS_PHASE_WRITE_DATA,
	KCS_PHASE_WRITE_END_CMD,
	KCS_PHASE_WRITE_DONE,

	KCS_PHASE_WAIT_READ,
	KCS_PHASE_READ,

	KCS_PHASE_ABORT_ERROR1,
	KCS_PHASE_ABORT_ERROR2,
	KCS_PHASE_ERROR,

	KCS_PHASE_NUM
};

static void kcs_set_state(struct kcs_aspeed_data *kcs, enum kcs_state stat)
{
	uint32_t reg;

	reg = LPC_RD(kcs->str);
	reg &= ~(KCS_STR_STATE_MASK);
	reg |= (stat << KCS_STR_STATE_SHIFT) & KCS_STR_STATE_MASK;
	LPC_WR(reg, kcs->str);
}

static void kcs_write_data(struct kcs_aspeed_data *kcs, uint8_t data)
{
	LPC_WR(data, kcs->odr);
}

static uint8_t kcs_read_data(struct kcs_aspeed_data *kcs)
{
	return LPC_RD(kcs->idr);
}

static void kcs_force_abort(struct kcs_aspeed_data *kcs)
{
	kcs_set_state(kcs, KCS_STATE_ERROR);
	kcs_read_data(kcs);
	kcs_write_data(kcs, KCS_DUMMY_ZERO);
	kcs->ibuf_avail = 0;
	kcs->ibuf_idx = 0;
	kcs->phase = KCS_PHASE_ERROR;
}

static void kcs_handle_cmd(struct kcs_aspeed_data *kcs)
{
	uint8_t cmd;

	kcs_set_state(kcs, KCS_STATE_WRITE);
	kcs_write_data(kcs, KCS_DUMMY_ZERO);

	cmd = kcs_read_data(kcs);
	switch (cmd) {
	case KCS_CMD_WRITE_START:
		kcs->phase = KCS_PHASE_WRITE_START;
		kcs->error = KCS_NO_ERROR;
		break;

	case KCS_CMD_WRITE_END:
		if (kcs->phase != KCS_PHASE_WRITE_DATA) {
			kcs_force_abort(kcs);
			break;
		}
		kcs->phase = KCS_PHASE_WRITE_END_CMD;
		break;

	case KCS_CMD_GET_STATUS_ABORT:
		if (kcs->error == KCS_NO_ERROR) {
			kcs->error = KCS_ABORTED_BY_COMMAND;
		}

		kcs->phase = KCS_PHASE_ABORT_ERROR1;
		break;

	default:
		kcs_force_abort(kcs);
		kcs->error = KCS_ILLEGAL_CONTROL_CODE;
		break;
	}
}

static void kcs_handle_data(struct kcs_aspeed_data *kcs)
{
	uint8_t data;

	switch (kcs->phase) {
	case KCS_PHASE_WRITE_START:
		kcs->phase = KCS_PHASE_WRITE_DATA;
	/* fall through */
	case KCS_PHASE_WRITE_DATA:
		if (kcs->ibuf_idx < KCS_BUF_SIZE) {
			kcs_set_state(kcs, KCS_STATE_WRITE);
			kcs_write_data(kcs, KCS_DUMMY_ZERO);
			kcs->ibuf[kcs->ibuf_idx] = kcs_read_data(kcs);
			kcs->ibuf_idx++;
		} else {
			kcs_force_abort(kcs);
			kcs->error = KCS_LENGTH_ERROR;
		}
		break;

	case KCS_PHASE_WRITE_END_CMD:
		if (kcs->ibuf_idx < KCS_BUF_SIZE) {
			kcs_set_state(kcs, KCS_STATE_READ);
			kcs->ibuf[kcs->ibuf_idx] = kcs_read_data(kcs);
			kcs->ibuf_idx++;
			kcs->ibuf_avail = 1;
			kcs->phase = KCS_PHASE_WRITE_DONE;
		} else {
			kcs_force_abort(kcs);
			kcs->error = KCS_LENGTH_ERROR;
		}
		break;

	case KCS_PHASE_READ:
		if (kcs->obuf_idx == kcs->obuf_data_sz) {
			kcs_set_state(kcs, KCS_STATE_IDLE);
		}

		data = kcs_read_data(kcs);
		if (data != KCS_CMD_READ_BYTE) {
			kcs_set_state(kcs, KCS_STATE_ERROR);
			kcs_write_data(kcs, KCS_DUMMY_ZERO);
			break;
		}

		if (kcs->obuf_idx == kcs->obuf_data_sz) {
			kcs_write_data(kcs, KCS_DUMMY_ZERO);
			kcs->phase = KCS_PHASE_IDLE;
			break;
		}

		kcs_write_data(kcs, kcs->obuf[kcs->obuf_idx]);
		kcs->obuf_idx++;
		break;

	case KCS_PHASE_ABORT_ERROR1:
		kcs_set_state(kcs, KCS_STATE_READ);
		kcs_read_data(kcs);
		kcs_write_data(kcs, kcs->error);
		kcs->phase = KCS_PHASE_ABORT_ERROR2;
		break;

	case KCS_PHASE_ABORT_ERROR2:
		kcs_set_state(kcs, KCS_STATE_IDLE);
		kcs_read_data(kcs);
		kcs_write_data(kcs, KCS_DUMMY_ZERO);
		kcs->phase = KCS_PHASE_IDLE;

	default:
		kcs_force_abort(kcs);
		break;
	}

}

static void kcs_aspeed_isr(const struct device *dev)
{
	uint32_t stat;
	struct kcs_aspeed_data *kcs = (struct kcs_aspeed_data *)dev->data;

	stat = LPC_RD(kcs->str);
	if (stat & KCS_STR_IBF) {
		if (stat & KCS_STR_CMD_DAT) {
			kcs_handle_cmd(kcs);
		} else {
			kcs_handle_data(kcs);
		}
	}
}

static int kcs_aspeed_config_ioregs(const struct device *dev)
{
	struct kcs_aspeed_data *kcs = (struct kcs_aspeed_data *)dev->data;
	struct kcs_aspeed_config *cfg = (struct kcs_aspeed_config *)dev->config;

	switch (cfg->chan) {
	case KCS_CH1:
		kcs->idr = IDR1;
		kcs->odr = ODR1;
		kcs->str = STR1;
		break;
	case KCS_CH2:
		kcs->idr = IDR2;
		kcs->odr = ODR2;
		kcs->str = STR2;
		break;
	case KCS_CH3:
		kcs->idr = IDR3;
		kcs->odr = ODR3;
		kcs->str = STR3;
		break;
	case KCS_CH4:
		kcs->idr = IDR4;
		kcs->odr = ODR4;
		kcs->str = STR4;
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int kcs_aspeed_config_addr(const struct device *dev)
{
	uint32_t reg;
	struct kcs_aspeed_config *cfg = (struct kcs_aspeed_config *)dev->config;

	switch (cfg->chan) {
	case KCS_CH1:
		reg = LPC_RD(HICR4);
		reg &= ~(HICR4_LADR12AS);
		LPC_WR(reg, HICR4);
		LPC_WR(cfg->addr >> 8, LADR12H);
		LPC_WR(cfg->addr & 0xff, LADR12L);
		break;
	case KCS_CH2:
		reg = LPC_RD(HICR4);
		reg |= HICR4_LADR12AS;
		LPC_WR(reg, HICR4);
		LPC_WR(cfg->addr >> 8, LADR12H);
		LPC_WR(cfg->addr & 0xff, LADR12L);
		break;
	case KCS_CH3:
		LPC_WR(cfg->addr >> 8, LADR3H);
		LPC_WR(cfg->addr & 0xff, LADR3L);
		break;
	case KCS_CH4:
		reg = ((cfg->addr + 1) << 16) | (cfg->addr & 0xffff);
		LPC_WR(reg, LADR4);
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int kcs_aspeed_enable_chan(const struct device *dev)
{
	uint32_t reg;
	struct kcs_aspeed_config *cfg = (struct kcs_aspeed_config *)dev->config;

	switch (cfg->chan) {
	case KCS_CH1:
		reg = LPC_RD(HICR2) | HICR2_IBFIF1;
		LPC_WR(reg, HICR2);
		reg = LPC_RD(HICR0) | HICR0_LPC1E;
		LPC_WR(reg, HICR0);
		break;
	case KCS_CH2:
		reg = LPC_RD(HICR2) | HICR2_IBFIF2;
		LPC_WR(reg, HICR2);
		reg = LPC_RD(HICR0) | HICR0_LPC2E;
		LPC_WR(reg, HICR0);
		break;
	case KCS_CH3:
		reg = LPC_RD(HICR2) | HICR2_IBFIF3;
		LPC_WR(reg, HICR2);
		reg = LPC_RD(HICR0) | HICR0_LPC3E;
		LPC_WR(reg, HICR0);
		break;
	case KCS_CH4:
		reg = LPC_RD(HICRB) | HICRB_IBFIF4 | HICRB_LPC4E;
		LPC_WR(reg, HICRB);
		reg = LPC_RD(STR4) & ~STR4_STAT_MASK;
		LPC_WR(reg, STR4);
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

int kcs_aspeed_read(const struct device *dev,
		    uint8_t *buf, uint32_t buf_sz)
{
	int ret;
	struct kcs_aspeed_data *kcs = (struct kcs_aspeed_data *)dev->data;

	if (kcs == NULL || buf == NULL) {
		return -EINVAL;
	}

	if (buf_sz < kcs->ibuf_idx) {
		return -ENOSPC;
	}

	if (!kcs->ibuf_avail) {
		return -ENODATA;
	}

	if (kcs->phase != KCS_PHASE_WRITE_DONE) {
		kcs_force_abort(kcs);
		return -EPERM;
	}

	memcpy(buf, kcs->ibuf, kcs->ibuf_idx);
	ret = kcs->ibuf_idx;

	kcs->phase = KCS_PHASE_WAIT_READ;
	kcs->ibuf_avail = 0;
	kcs->ibuf_idx = 0;

	return ret;
}

int kcs_aspeed_write(const struct device *dev,
		     uint8_t *buf, uint32_t buf_sz)
{
	struct kcs_aspeed_data *kcs = (struct kcs_aspeed_data *)dev->data;

	/* a minimum response size is 3: netfn + cmd + cmplt_code */
	if (buf_sz < 3 || buf_sz > KCS_BUF_SIZE) {
		return -EINVAL;
	}

	if (kcs->phase != KCS_PHASE_WAIT_READ) {
		return -EPERM;
	}

	kcs->phase = KCS_PHASE_READ;
	kcs->obuf_idx = 1;
	kcs->obuf_data_sz = buf_sz;
	memcpy(kcs->obuf, buf, buf_sz);
	kcs_write_data(kcs, kcs->obuf[0]);

	return buf_sz;
}

static int kcs_aspeed_init(const struct device *dev)
{
	int rc;
	struct kcs_aspeed_data *kcs = (struct kcs_aspeed_data *)dev->data;
	struct kcs_aspeed_config *cfg = (struct kcs_aspeed_config *)dev->config;

	if (!lpc_base) {
		lpc_base = cfg->base;
	}

	kcs->ibuf_idx = 0;
	kcs->ibuf_avail = 0;

	kcs->obuf_idx = 0;
	kcs->obuf_data_sz = 0;

	kcs->phase = KCS_PHASE_IDLE;

	rc = kcs_aspeed_config_ioregs(dev);
	if (rc) {
		LOG_ERR("failed to config IO regs, rc=%d\n", rc);
		return rc;
	}

	rc = kcs_aspeed_config_addr(dev);
	if (rc) {
		LOG_ERR("failed to config IO address, rc=%d\n", rc);
		return rc;
	}

	cfg->irq_config_func(dev);

	rc = kcs_aspeed_enable_chan(dev);
	if (rc) {
		LOG_ERR("failed to enable KCS#%d, rc=%d\n", cfg->chan, rc);
		return rc;
	}

	LOG_INF("KCS%d: addr=0x%x, idr=0x%x, odr=0x%x, str=0x%x\n",
		cfg->chan, cfg->addr,
		kcs->idr, kcs->odr, kcs->str);

	return 0;
}

#define KCS_ASPEED_INIT(n)						     \
	static struct kcs_aspeed_data kcs_aspeed_data_##n;		     \
									     \
	static void kcs_aspeed_irq_config_func_##n(const struct device *dev) \
	{								     \
		ARG_UNUSED(dev);					     \
									     \
		IRQ_CONNECT(DT_INST_IRQN(n),				     \
			    DT_INST_IRQ(n, priority),			     \
			    kcs_aspeed_isr, DEVICE_DT_INST_GET(n), 0);	     \
									     \
		irq_enable(DT_INST_IRQN(n));				     \
	}								     \
									     \
	static const struct kcs_aspeed_config kcs_aspeed_config_##n = {	     \
		.base = DT_REG_ADDR(DT_PARENT(DT_DRV_INST(n))),		     \
		.chan = DT_INST_PROP(n, chan),				     \
		.addr = DT_INST_PROP(n, addr),				     \
		.irq_config_func = kcs_aspeed_irq_config_func_##n,	     \
	};								     \
									     \
	DEVICE_DT_INST_DEFINE(n,					     \
			      kcs_aspeed_init,				     \
			      NULL,					     \
			      &kcs_aspeed_data_##n,			     \
			      &kcs_aspeed_config_##n,			     \
			      POST_KERNEL,				     \
			      CONFIG_KERNEL_INIT_PRIORITY_DEVICE,	     \
			      NULL);

DT_INST_FOREACH_STATUS_OKAY(KCS_ASPEED_INIT)
