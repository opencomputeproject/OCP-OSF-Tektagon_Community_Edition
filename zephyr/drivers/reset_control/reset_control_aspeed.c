/*
 * Copyright (c) 2021, ASPEED
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT aspeed_aspeed_rst
#include <errno.h>
#include <soc.h>
#include <drivers/reset_control.h>

#define LOG_LEVEL CONFIG_RESET_CONTROL_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(reset_control);

struct reset_aspeed_config {
	uintptr_t base;
};

#define DEV_CFG(dev)				   \
	((const struct reset_aspeed_config *const) \
	 (dev)->config)

#define GET_ASSERT_OFFSET(rst_id) ((rst_id >> 16) & 0xff)
#define GET_DEASSERT_OFFSET(rst_id) ((rst_id >> 8) & 0xff)
#define GET_RST_BIT(rst_id) ((rst_id) & 0xff)


static int aspeed_reset_control_deassert(const struct device *dev,
					 reset_control_subsys_t sub_system)
{
	uint32_t rst_id = (uint32_t) sub_system;
	uint32_t scu_base = DEV_CFG(dev)->base;

	sys_write32(BIT(GET_RST_BIT(rst_id)),
		    scu_base + GET_DEASSERT_OFFSET(rst_id));
	LOG_DBG("Deassert offset:0x%08x bit:%d", GET_DEASSERT_OFFSET(rst_id),
		GET_RST_BIT(rst_id));
	return 0;
}

static int aspeed_reset_control_assert(const struct device *dev,
				       reset_control_subsys_t sub_system)
{
	uint32_t rst_id = (uint32_t) sub_system;
	uint32_t scu_base = DEV_CFG(dev)->base;

	sys_write32(BIT(GET_RST_BIT(rst_id)),
		    scu_base + GET_ASSERT_OFFSET(rst_id));
	LOG_DBG("Assert offset:0x%08x bit:%d", GET_ASSERT_OFFSET(rst_id),
		GET_RST_BIT(rst_id));
	return 0;
}


static int aspeed_reset_control_init(const struct device *dev)
{
	return 0;
}

static const struct reset_control_driver_api aspeed_rst_api = {
	.deassert = aspeed_reset_control_deassert,
	.assert = aspeed_reset_control_assert,
};

static const struct reset_aspeed_config reset_aspeed_cfg = {
	.base = DT_REG_ADDR(DT_NODELABEL(syscon)),
};

#define ASPEED_RESET_INIT(n)							\
	DEVICE_DT_INST_DEFINE(n,						\
			      &aspeed_reset_control_init,			\
			      NULL,						\
			      NULL, &reset_aspeed_cfg,				\
			      PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,	\
			      &aspeed_rst_api);

DT_INST_FOREACH_STATUS_OKAY(ASPEED_RESET_INIT)
