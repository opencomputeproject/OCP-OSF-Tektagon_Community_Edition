/*
 * Copyright (c) 2021 Aspeed Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#define DT_DRV_COMPAT aspeed_i2c_mbx

#include <sys/util.h>
#include <sys/slist.h>
#include <kernel.h>
#include <errno.h>
#include <drivers/i2c.h>
#include <soc.h>
#include <device.h>
#include <string.h>
#include <stdlib.h>
#include <drivers/i2c/pfr/i2c_mailbox.h>

#define LOG_LEVEL CONFIG_I2C_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(i2c_pfr_mbx);

/* #define ASPEED_I2C_MJ_DUMP */
#define ASPEED_I2C_MW_DUMP

#ifdef ASPEED_I2C_MJ_DUMP
#define I2C_W_R(value, addr) LOG_INF("  dw %x %x", addr, value);
#define I2C_LW_R(value, addr) LOG_INF("  dw %x %lx", addr, value);
#else
#ifdef ASPEED_I2C_MW_DUMP
#define I2C_W_R(value, addr) LOG_INF("  dw %x %x\n", addr, value); sys_write32(value, addr);
#define I2C_LW_R(value, addr) LOG_INF("  dw %x %lx", addr, value); sys_write32(value, addr);
#else
#define I2C_W_R(value, addr) sys_write32(value, addr);
#define I2C_LW_R(value, addr) sys_write32(value, addr);
#endif
#endif

struct ast_i2c_mbx_data {
	uint32_t	i2c_dev_base[AST_I2C_M_DEV_COUNT];	/* i2c dev base*/
	uint8_t	mail_addr_en[AST_I2C_M_DEV_COUNT];	/* mbx addr enable*/
	uint8_t	mail_en[AST_I2C_M_DEV_COUNT];		/* mbx enable*/
	uint32_t	i2c_s_ier;							/* slave ier keep */
};

struct ast_i2c_mbx_config {
	char		*mail_dev_name;
	uint32_t	mail_g_base;
	void (*irq_config_func)(const struct device *dev);
};

/* convenience defines */
#define DEV_CFG(dev)				     \
	((const struct ast_i2c_mbx_config *const) \
	 (dev)->config)
#define DEV_DATA(dev) \
	((struct ast_i2c_mbx_data *const)(dev)->data)

/* check common parameter valid */
int check_ast_mbx_valid(const struct ast_i2c_mbx_config *cfg, uint8_t dev_idx)
{
	if (!cfg->mail_dev_name) {
		LOG_ERR("i2c mailbox not found");
		return -EINVAL;
	} else if ((dev_idx != 0) &&
			(dev_idx != 1)) {
		LOG_ERR("invalid i2c mailbox index");
		return -EINVAL;
	}
	return 0;
}

/* i2c mbx interrupt service routine */
void ast_i2c_mbx_isr(const struct device *dev)
{
	const struct ast_i2c_mbx_config *cfg = DEV_CFG(dev);

	uint32_t sts0 = sys_read32(cfg->mail_g_base + AST_I2C_M_IRQ_STA0);
	uint32_t sts1 = sys_read32(cfg->mail_g_base + AST_I2C_M_IRQ_STA1);
	uint32_t stsfifo = sys_read32(cfg->mail_g_base + AST_I2C_M_FIFO_IRQ);

	LOG_INF(" B mail sts0 : %x", sts0);
	LOG_INF(" B mail sts1 : %x", sts1);
	LOG_INF(" B mail stsfifo : %x", stsfifo);

	I2C_W_R(sts0, cfg->mail_g_base + AST_I2C_M_IRQ_STA0);
	I2C_W_R(sts1, cfg->mail_g_base + AST_I2C_M_IRQ_STA1);
	I2C_W_R(stsfifo, cfg->mail_g_base + AST_I2C_M_FIFO_IRQ);

	sts0 = sys_read32(cfg->mail_g_base + AST_I2C_M_IRQ_STA0);
	sts1 = sys_read32(cfg->mail_g_base + AST_I2C_M_IRQ_STA1);
	stsfifo = sys_read32(cfg->mail_g_base + AST_I2C_M_FIFO_IRQ);

	LOG_INF(" A mail sts0 : %x", sts0);
	LOG_INF(" A mail sts1 : %x", sts1);
	LOG_INF(" A mail stsfifo : %x", stsfifo);
}

int ast_i2c_mbx_fifo_access(const struct device *dev, uint8_t idx,
uint8_t type, uint8_t *data)
{
	const struct ast_i2c_mbx_config *cfg = DEV_CFG(dev);
	uint32_t addr = 0;

	/* check common parameter valid */
	if (!cfg->mail_dev_name) {
		LOG_ERR("i2c mailbox not found");
		return -EINVAL;
	}

	/* check access type */
	if ((type != AST_I2C_M_R) &&
	(type != AST_I2C_M_W))
		return -EINVAL;

	/* fill access base */
	if (idx == 0) {
		addr = (cfg->mail_g_base & 0xFFFF0000) + FIFO0_ACCESS;
	} else if (idx == 1) {
		addr = (cfg->mail_g_base & 0xFFFF0000) + FIFO1_ACCESS;
	} else {
		return -EINVAL;
	}

	/* do access */
	if (type == AST_I2C_M_R) {
		*data = sys_read32(addr) & 0xFF;
	} else {
		sys_write32(*data, addr);
	}

	return 0;
}

int ast_i2c_mbx_fifo_priority(const struct device *dev, uint8_t priority)
{
	const struct ast_i2c_mbx_config *cfg = DEV_CFG(dev);

	uint32_t value;

	/* check common parameter valid */
	if (!cfg->mail_dev_name) {
		LOG_ERR("i2c mailbox not found");
		return -EINVAL;
	}

	/* invalid priority */
	if (priority > AST_I2C_M_I2C1)
		return -EINVAL;

	/* apply fifo config */
	value = (sys_read32(cfg->mail_g_base + AST_I2C_M_CFG) &
		~(AST_I2C_M_FIFO_I2C1_H | AST_I2C_M_FIFO_I2C2_H));

	if (priority == AST_I2C_M_I2C1) {
		value |= AST_I2C_M_FIFO_I2C1_H;
	} else if (priority == AST_I2C_M_I2C2) {
		value |= AST_I2C_M_FIFO_I2C2_H;
	}

	I2C_W_R(value, cfg->mail_g_base + AST_I2C_M_CFG);

	return 0;
}


/* i2c mbx fifo apply setting */
int ast_i2c_mbx_fifo_apply(const struct device *dev, uint8_t idx,
uint8_t addr, uint8_t type)
{
	const struct ast_i2c_mbx_config *cfg = DEV_CFG(dev);

	uint32_t value;

	/* check common parameter valid */
	if (!cfg->mail_dev_name) {
		LOG_ERR("i2c mailbox not found");
		return -EINVAL;
	}

	/* invalid type */
	if (type > (AST_I2C_M_R | AST_I2C_M_W))
		return -EINVAL;

	/* apply fifo config */
	value = (sys_read32(cfg->mail_g_base + AST_I2C_M_CFG)
		| AST_I2C_M_FIFO_R_NAK);

	if (idx == 0) {
		value &= AST_I2C_M_FIFO0_MASK;
		value |= (AST_I2C_M_FIFO_REMAP | addr);

		if (type & AST_I2C_M_W)
			value |= AST_I2C_M_FIFO0_W_DIS;

		if (type & AST_I2C_M_R)
			value |= AST_I2C_M_FIFO0_R_DIS;
	} else if (idx == 1) {
		value &= AST_I2C_M_FIFO1_MASK;
		value |= (AST_I2C_M_FIFO_REMAP | (addr << 8));

		if (type & AST_I2C_M_W)
			value |= AST_I2C_M_FIFO1_W_DIS;

		if (type & AST_I2C_M_R)
			value |= AST_I2C_M_FIFO1_R_DIS;
	} else {
		return -EINVAL;
	}

	I2C_W_R(value, cfg->mail_g_base + AST_I2C_M_CFG);

	return 0;
}

/* i2c mbx fifo enable */
int ast_i2c_mbx_fifo_en(const struct device *dev, uint8_t idx,
uint16_t base, uint16_t length)
{
	const struct ast_i2c_mbx_config *cfg = DEV_CFG(dev);

	uint32_t value = (base + FIFO_BASE) | (length << 16);
	uint32_t fifo_int = 0, value1;

	/* check common parameter valid */
	if (!cfg->mail_dev_name) {
		LOG_ERR("i2c mailbox not found");
		return -EINVAL;
	}

	/* check fifo length limit if length not equal 0*/
	if (length) {
		if (base + length > FIFO_MAX_SIZE)
			return -EINVAL;
	}

	/* clear interrupt status and keep original interrupt setting */
	value1 = sys_read32(cfg->mail_g_base + AST_I2C_M_FIFO_IRQ);
	I2C_W_R(value1, cfg->mail_g_base + AST_I2C_M_FIFO_IRQ);

	fifo_int = value1 & AST_I2C_M_FIFO_INT;

	if (idx == 0) {
		/* length is used as enable */
		if (length) {
			fifo_int |= AST_I2C_M_FIFO0_INT_EN;
		} else {
			fifo_int &= ~(AST_I2C_M_FIFO0_INT_EN);
		}
		I2C_W_R(fifo_int, cfg->mail_g_base + AST_I2C_M_FIFO_IRQ);

		I2C_W_R(value, cfg->mail_g_base + AST_I2C_M_FIFO_CFG0);
	} else if (idx == 1) {
		/* length is used as enable */
		if (length) {
			fifo_int |= AST_I2C_M_FIFO1_INT_EN;
		} else {
			fifo_int &= ~(AST_I2C_M_FIFO1_INT_EN);
		}
		I2C_W_R(fifo_int, cfg->mail_g_base + AST_I2C_M_FIFO_IRQ);

		I2C_W_R(value, cfg->mail_g_base + AST_I2C_M_FIFO_CFG1);
	} else {
		return -EINVAL;
	}

	return 0;
}

/* i2c mbx notify enable */
int ast_i2c_mbx_notify_en(const struct device *dev, uint8_t dev_idx,
uint8_t idx, uint8_t type, uint8_t enable)
{
	const struct ast_i2c_mbx_config *cfg = DEV_CFG(dev);

	uint8_t reg_offset = AST_I2C_M_IRQ_EN0;
	uint32_t value = 0;

	/* check common parameter valid */
	if (check_ast_mbx_valid(cfg, dev_idx))
		return -EINVAL;

	/* over notify index define */
	if (idx > 0xF)
		return -EINVAL;

	/* invalid type */
	if ((type < AST_I2C_M_R) ||
	(type > (AST_I2C_M_R | AST_I2C_M_W)))
		return -EINVAL;

	if (dev_idx == 1)
		reg_offset = AST_I2C_M_IRQ_EN1;

	/* calculate the interrupt flag position */
	value = sys_read32(cfg->mail_g_base + reg_offset);

	if (enable) {
		if (type & AST_I2C_M_R)
			value |= (0x1 << (idx + 0x10));

		if (type & AST_I2C_M_W)
			value |= (0x1 << idx);
	} else {
		if (type & AST_I2C_M_R)
			value &= ~((0x1 << (idx + 0x10)));

		if (type & AST_I2C_M_W)
			value &= ~(0x1 << idx);
	}

	I2C_W_R(value, cfg->mail_g_base + reg_offset);

	return 0;
}


/* i2c mbx notify address */
int ast_i2c_mbx_notify_addr(const struct device *dev, uint8_t idx,
uint8_t addr)
{
	const struct ast_i2c_mbx_config *cfg = DEV_CFG(dev);

	uint8_t reg_index, reg_offset = AST_I2C_M_ADDR_IRQ0;
	uint32_t value, byteshift;

	/* check common parameter valid */
	if (!cfg->mail_dev_name) {
		LOG_ERR("i2c mailbox not found");
		return -EINVAL;
	}

	/* over notify index define */
	if (idx > 0xF)
		return -EINVAL;

	/* calculte nodify address position */
	reg_index = idx >> 0x2;
	byteshift = ((idx % 0x4) << 3);

	reg_offset += (0x4 * reg_index);

	value = (sys_read32(cfg->mail_g_base + reg_offset)
	& ~(0xFF << byteshift));

	value |= (addr << byteshift);

	I2C_W_R(value, cfg->mail_g_base + reg_offset);

	return 0;
}


/* i2c mbx protect address */
int ast_i2c_mbx_protect(const struct device *dev, uint8_t dev_idx,
uint8_t addr, uint8_t enable)
{
	const struct ast_i2c_mbx_config *cfg = DEV_CFG(dev);

	uint8_t index, bit;
	uint32_t value, base = AST_I2C_M_WP_BASE0;

	/* check common parameter valid */
	if (check_ast_mbx_valid(cfg, dev_idx))
		return -EINVAL;

	/* change write protect base */
	if (dev_idx == 0x1)
		base = AST_I2C_M_WP_BASE1;

	/* calculte bitmap position */
	index = addr / 0x20;
	bit = addr % 0x20;

	base += (index << 2);

	value = sys_read32(cfg->mail_g_base + base);

	if (enable)
		value |= (0x1 << bit);
	else
		value = value & ~(0x1 << bit);

	I2C_W_R(value, cfg->mail_g_base + base);

	return 0;
}

/* i2c mbx enable */
int ast_i2c_mbx_en(const struct device *dev, uint8_t dev_idx,
uint8_t enable)
{
	struct ast_i2c_mbx_data *data = DEV_DATA(dev);
	const struct ast_i2c_mbx_config *cfg = DEV_CFG(dev);
	uint32_t	dev_base = 0;
	uint32_t value = 0;

	/* check common parameter valid */
	if (check_ast_mbx_valid(cfg, dev_idx))
		return -EINVAL;

	/* set mbx slave address before enable it */
	if (enable && (!(data->mail_addr_en[dev_idx])))
		return -EINVAL;

	/* change device base */
	dev_base = data->i2c_dev_base[dev_idx];

	/* set mbx base and length */
	if (enable) {
		I2C_W_R(AST_I2C_M_OFFSET, dev_base + AST_I2C_RX_DMA);
		I2C_W_R(AST_I2C_M_OFFSET, dev_base + AST_I2C_TX_DMA);
		I2C_W_R(0x400, dev_base + AST_I2C_MBX_LIM);

		/* save and clear slave interrupt */
		data->i2c_s_ier = sys_read32(dev_base + AST_I2C_S_IER);
		I2C_W_R(0x0, dev_base + AST_I2C_S_IER);

		value = AST_I2C_MBX_RX_DMA_LEN(AST_I2C_M_LENGTH);
		I2C_W_R(value, dev_base + AST_I2C_DMA_LEN);
		value = AST_I2C_MBX_TX_DMA_LEN(AST_I2C_M_LENGTH);
		I2C_W_R(value, dev_base + AST_I2C_DMA_LEN);
		value = (sys_read32(dev_base + AST_I2C_CTL)
			|(AST_I2C_MBX_EN|AST_I2C_SLAVE_EN));
		I2C_W_R(value, dev_base + AST_I2C_CTL);
	} else {
		value = (sys_read32(dev_base + AST_I2C_CTL)
		& ~(AST_I2C_MBX_EN|AST_I2C_SLAVE_EN));
		I2C_W_R(value, dev_base + AST_I2C_CTL);

		/* restore the slave interrupt */
		I2C_W_R(data->i2c_s_ier, dev_base + AST_I2C_S_IER);

		/* reset dma offset */
		value = (sys_read32(dev_base + AST_I2C_CTL)
		& (~AST_I2C_MASTER_EN));
		I2C_W_R(value, dev_base + AST_I2C_CTL);
		value |= AST_I2C_MASTER_EN;
		I2C_W_R(value, dev_base + AST_I2C_CTL);
	}

	data->mail_en[dev_idx] = enable;

	return 0;
}

/* i2c mbx set addr */
int ast_i2c_mbx_addr(const struct device *dev, uint8_t dev_idx,
uint8_t idx, uint8_t offset, uint8_t addr, uint8_t enable)
{
	struct ast_i2c_mbx_data *data = DEV_DATA(dev);
	const struct ast_i2c_mbx_config *cfg = DEV_CFG(dev);
	uint32_t	dev_base = 0;
	uint32_t mask_addr = 0;

	/* check common parameter valid */
	if (check_ast_mbx_valid(cfg, dev_idx))
		return -EINVAL;

	/* change device base */
	dev_base = data->i2c_dev_base[dev_idx];

	/* Set slave addr. */
	switch (idx) {
	case 0:
		mask_addr = (sys_read32(dev_base + AST_I2C_ADDR)
			& ~(AST_I2CS_ADDR1_CLEAR));
		if (enable) {
			mask_addr |= (AST_I2CS_ADDR1(addr)|
			AST_I2CS_ADDR1_MBX_TYPE(offset)|AST_I2CS_ADDR1_ENABLE);
		}
		break;
	case 1:
		mask_addr = (sys_read32(dev_base + AST_I2C_ADDR)
			& ~(AST_I2CS_ADDR2_CLEAR));
		if (enable) {
			mask_addr |= (AST_I2CS_ADDR2(addr)|
			AST_I2CS_ADDR2_MBX_TYPE(offset)|AST_I2CS_ADDR2_ENABLE);
		}
		break;
	case 2:
		mask_addr = (sys_read32(dev_base + AST_I2C_ADDR)
			& ~(AST_I2CS_ADDR3_CLEAR));
		if (enable) {
			mask_addr |= (AST_I2CS_ADDR3(addr)|
			AST_I2CS_ADDR3_MBX_TYPE(offset)|AST_I2CS_ADDR3_ENABLE);
		}
		break;
	default:
		return -EINVAL;
	}

	/* fire in register */
	I2C_W_R(mask_addr, dev_base + AST_I2C_ADDR);

	/* find out if any one i2c slave is existed */
	mask_addr = (sys_read32(dev_base + AST_I2C_ADDR)
		& (AST_I2CS_ADDR1_ENABLE|
		AST_I2CS_ADDR2_ENABLE|
		AST_I2CS_ADDR3_ENABLE));

	if (mask_addr) {
		data->mail_addr_en[dev_idx] = 1;
	} else {
		data->mail_addr_en[dev_idx] = 0;
	}

	return 0;
}

/* i2c mbx initial */
int ast_i2c_mbx_init(const struct device *dev)
{
	struct ast_i2c_mbx_data *data = DEV_DATA(dev);
	const struct ast_i2c_mbx_config *cfg = DEV_CFG(dev);

	uint32_t sts = 0;

	/* clear mbx addr /fifo irq status */
	I2C_W_R(0xFFFFFFFF, (cfg->mail_g_base+AST_I2C_M_IRQ_STA0));
	I2C_W_R(0xFFFFFFFF, (cfg->mail_g_base+AST_I2C_M_IRQ_STA1));

	sts =  sys_read32(cfg->mail_g_base + AST_I2C_M_FIFO_IRQ);
	I2C_W_R(sts, (cfg->mail_g_base+AST_I2C_M_FIFO_IRQ));

	/* i2c device base for slave setting */
	data->i2c_dev_base[0] = (cfg->mail_g_base & 0xFFFF0000)+0x80;
	data->i2c_dev_base[1] = (cfg->mail_g_base & 0xFFFF0000)+0x100;

	/* hook interrupt routine*/
	cfg->irq_config_func(dev);

	/* temp clear internal SRAM for mailbox verified */
	sts = 0x7e7b0e00;
	memset((uint32_t *)sts, 0, 0x200);

	return 0;
}

#define I2C_MBX_INIT(inst)				 \
	static void ast_i2c_mbx_cfg_##inst(const struct device *dev);	 \
			 \
	static struct ast_i2c_mbx_data			 \
		ast_i2c_mbx_##inst##_dev_data;	 \
			 \
	static const struct ast_i2c_mbx_config		 \
		ast_i2c_mbx_##inst##_cfg = {		 \
		.mail_dev_name = DT_INST_PROP(inst, label),	 \
		.mail_g_base = DT_INST_REG_ADDR(inst),	 \
		.irq_config_func = ast_i2c_mbx_cfg_##inst,	 \
	};		 \
			 \
	DEVICE_DT_INST_DEFINE(inst,		 \
			      &ast_i2c_mbx_init,	 \
			      NULL,				 \
			      &ast_i2c_mbx_##inst##_dev_data,	 \
			      &ast_i2c_mbx_##inst##_cfg,		 \
			      POST_KERNEL,			 \
			      CONFIG_KERNEL_INIT_PRIORITY_DEVICE,	 \
			      NULL);								 \
			 \
	static void ast_i2c_mbx_cfg_##inst(const struct device *dev) \
	{									 \
		ARG_UNUSED(dev);					 \
										 \
		IRQ_CONNECT(DT_INST_IRQN(inst),	 \
				DT_INST_IRQ(inst, priority),	 \
				ast_i2c_mbx_isr, DEVICE_DT_INST_GET(inst), 0); \
			 \
		irq_enable(DT_INST_IRQN(inst));		 \
	}

DT_INST_FOREACH_STATUS_OKAY(I2C_MBX_INIT)
