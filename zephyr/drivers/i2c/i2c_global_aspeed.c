/*
 * Copyright (c) ASPEED Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT aspeed_i2c_global

#include <drivers/clock_control.h>
#include <drivers/reset_control.h>
#include <drivers/i2c.h>
#include "soc.h"

#include <logging/log.h>
#define LOG_LEVEL CONFIG_I2C_LOG_LEVEL
LOG_MODULE_REGISTER(i2c_global);

#include <sys/sys_io.h>
#include <device.h>

#define ASPEED_I2CG_CONTROL		0x0C
#define ASPEED_I2CG_NEW_CLK_DIV	0x10

#define ASPEED_I2C_SRAM_BASE		0x7e7b0e00
#define ASPEED_I2C_SRAM_SIZE		0x80

#define CLK_NEW_MODE		BIT(1)
#define REG_NEW_MODE		BIT(2)
#define ISSUE_NAK_EMPTY	BIT(4)

#define I2CG_SET	(CLK_NEW_MODE |\
			REG_NEW_MODE |\
			ISSUE_NAK_EMPTY)

/* Device config */
struct i2c_global_config {
	uintptr_t base; /* i2c controller base address */
	const struct device *clock_dev;
	const clock_control_subsys_t clk_id;
	const reset_control_subsys_t rst_id;
	uint32_t clk_src;
};

#define DEV_CFG(dev)			 \
	((struct i2c_global_config *) \
	 (dev)->config)

#define BASE_CLK_COUNT	4

static uint32_t base_freq[BASE_CLK_COUNT] = {
	20000000,	/* 20M */
	10000000,	/* 10M */
	3250000,	/* 3.25M */
	1000000,	/* 1M */
};

/* I2C controller driver registration */
static uint32_t i2c_get_new_clk_divider(uint32_t base_clk)
{
	uint32_t i, j, base_freq_loop, clk_divider = 0;

	/* calculate clock divider */
	for (i = 0; i < BASE_CLK_COUNT; i++) {
		for (j = 0; j < 0xff ; j++) {
			/*
			 * j maps to div:
			 * 0x00: div 1
			 * 0x01: div 1.5
			 * 0x02: div 2
			 * 0x03: div 2.5
			 * 0x04: div 3
			 * ...
			 * 0xFE: div 128
			 * 0xFF: div 128.5
			 */
			base_freq_loop = base_clk * 2 / (j + 2);
			if (base_freq_loop <= base_freq[i])
				break;
		}
		LOG_DBG("i2c divider %d : 0x%x\n", i, j);
		clk_divider |= (j << (i << 3));
	}
	return clk_divider;
}

/* I2C controller driver registration */
static int i2c_global_init(const struct device *dev)
{
	struct i2c_global_config *config = DEV_CFG(dev);
	uint32_t i2c_global_base = config->base;
	uint32_t clk_divider = 0;
	uint32_t *base = (uint32_t *)ASPEED_I2C_SRAM_BASE;

	const struct device *reset_dev = device_get_binding(ASPEED_RST_CTRL_NAME);

	/* i2c controller reset / de-reset (delay is necessary) */
	reset_control_assert(reset_dev, DEV_CFG(dev)->rst_id);
	k_msleep(1);
	reset_control_deassert(reset_dev, DEV_CFG(dev)->rst_id);
	k_msleep(1);

	/* set i2c global setting */
	sys_write32(I2CG_SET, i2c_global_base + ASPEED_I2CG_CONTROL);

	/* calculate divider */
	clock_control_get_rate(config->clock_dev, config->clk_id, &config->clk_src);
	LOG_DBG("i2c global clk src %d\n", config->clk_src);

	clk_divider = i2c_get_new_clk_divider(config->clk_src);
	LOG_DBG("i2c clk divider %x\n", clk_divider);
	sys_write32(clk_divider, i2c_global_base + ASPEED_I2CG_NEW_CLK_DIV);

	/* initial i2c sram region */
	for (int i = 0; i < ASPEED_I2C_SRAM_SIZE; i++)
		*(base + i) = 0;

	return 0;
}

static const struct i2c_global_config i2c_aspeed_config = {
	.base = DT_REG_ADDR(DT_NODELABEL(i2c_gr)),
	.clk_id = (clock_control_subsys_t)DT_INST_CLOCKS_CELL(0, clk_id),
	.rst_id = (reset_control_subsys_t)DT_INST_RESETS_CELL(0, rst_id),
	.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(0)),
};


DEVICE_DT_INST_DEFINE(0, &i2c_global_init, NULL,
		      NULL, &i2c_aspeed_config,
		      POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		      NULL);
