/*
 * Copyright (c) ASPEED Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT aspeed_hace
#include <drivers/clock_control.h>
#include <drivers/reset_control.h>
#include "soc.h"
#include "hace_aspeed.h"

#define LOG_LEVEL CONFIG_CRYPTO_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(hace_global);

#include <sys/sys_io.h>
#include <device.h>

/* Device config */
struct hace_config {
	uintptr_t base; /* Hash and crypto engine base address */
	const struct device *clock_dev;
	const clock_control_subsys_t clk_id;
	const reset_control_subsys_t rst_id;
};

struct aspeed_hace_engine hace_eng;

#define DEV_CFG(dev)				 \
	((const struct hace_config *const) \
	 (dev)->config)

/* Crypto controller driver registration */
static int hace_init(const struct device *dev)
{
	uint32_t hace_base = DEV_CFG(dev)->base;
	const struct hace_config *config = DEV_CFG(dev);
	const struct device *reset_dev = device_get_binding(ASPEED_RST_CTRL_NAME);

	reset_control_assert(reset_dev, DEV_CFG(dev)->rst_id);
	k_usleep(100);
	clock_control_on(config->clock_dev, DEV_CFG(dev)->clk_id);
	k_msleep(10);
	reset_control_deassert(reset_dev, DEV_CFG(dev)->rst_id);
	hace_eng.base = (struct hace_register_s *)hace_base;

	return 0;
}

static const struct hace_config hace_aspeed_config = {
	.base = DT_REG_ADDR(DT_NODELABEL(hace)),
	.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(0)),
	.clk_id = (clock_control_subsys_t)DT_INST_CLOCKS_CELL(0, clk_id),
	.rst_id = (reset_control_subsys_t)DT_INST_RESETS_CELL(0, rst_id),
};

DEVICE_DT_INST_DEFINE(0, hace_init, NULL,
					  NULL, &hace_aspeed_config,
					  POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
					  NULL);
