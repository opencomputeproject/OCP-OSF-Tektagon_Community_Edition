/*
 * Copyright (c) 2021, ASPEED
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT fixed_factor_clock
#include <errno.h>
#include <soc.h>
#include <drivers/clock_control.h>
#if DT_HAS_COMPAT_STATUS_OKAY(fixed_factor_clock)
#define LOG_LEVEL CONFIG_CLOCK_CONTROL_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(fixed_factor_clock);

struct fixed_factor_clock_cfg {
	const struct device *clock_dev;
	const clock_control_subsys_t clk_id;
	const uint32_t clk_div;
	const uint32_t clk_mult;
};

#define DEV_CFG(dev) ((const struct fixed_factor_clock_cfg *const)(dev)->config)

static int fixed_factor_clock_control_get_rate(
	const struct device *dev,
	clock_control_subsys_t sub_system,
	uint32_t *rate)
{
	uint32_t src_rate;
	const struct fixed_factor_clock_cfg *config = DEV_CFG(dev);

	clock_control_get_rate(config->clock_dev, DEV_CFG(dev)->clk_id, &src_rate);

	LOG_DBG("source rate = %d, mult = %d, div = %d\n", src_rate,
		config->clk_mult, config->clk_div);
	*rate = (src_rate * config->clk_mult) / config->clk_div;

	return 0;
}

static int fixed_factor_clock_init(const struct device *dev)
{
	return 0;
}

static const struct clock_control_driver_api fixed_factor_clk_api = {
	.get_rate = fixed_factor_clock_control_get_rate,
};

#define FIXED_FACTOR_CLOCK_INIT(n)						  \
	static const struct fixed_factor_clock_cfg fixed_factor_clock_cfg_##n = { \
		.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(n)),		  \
		.clk_id = (clock_control_subsys_t)DT_INST_CLOCKS_CELL(n, clk_id), \
		.clk_div = DT_PROP_OR(DT_DRV_INST(n), clock_div, 1),		  \
		.clk_mult = DT_PROP_OR(DT_DRV_INST(n), clock_mult, 1),		  \
	};									  \
	DEVICE_DT_INST_DEFINE(n,						  \
			      fixed_factor_clock_init,				  \
			      NULL,						  \
			      NULL,						  \
			      &fixed_factor_clock_cfg_##n,			  \
			      PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,	  \
			      &fixed_factor_clk_api);

DT_INST_FOREACH_STATUS_OKAY(FIXED_FACTOR_CLOCK_INIT)
#endif /* DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT) */
