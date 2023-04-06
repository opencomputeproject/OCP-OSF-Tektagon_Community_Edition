/*
 * Copyright (c) 2021 ASPEED Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <device.h>
#include <kernel.h>
#include <soc.h>
#include <errno.h>
#include <string.h>
#include <logging/log.h>
#include <drivers/clock_control.h>
#include <drivers/gpio.h>
#include <drivers/misc/aspeed/pfr_aspeed.h>

#define PFR_SCU_CTRL_REG	0x7e6e20f0

#if !DT_NODE_HAS_STATUS(DT_INST(0, demo_gpio_basic_api), okay)
#error "no correct gpio device"
#endif

void pfr_bmc_srst_enable_ctrl(bool enable)
{
	int ret;
	const struct gpio_dt_spec gpio_m5 =
		GPIO_DT_SPEC_GET_BY_IDX(DT_INST(0, demo_gpio_basic_api),
						out_gpios, 0);

	if (enable)
		gpio_pin_set(gpio_m5.port, gpio_m5.pin, 0);
	else
		gpio_pin_set(gpio_m5.port, gpio_m5.pin, 1);

	ret = gpio_pin_configure_dt(&gpio_m5, GPIO_OUTPUT);
	if (ret)
		return;

	k_busy_wait(10000); /* 10ms */
}

#if DT_NODE_HAS_PROP(DT_INST(0, demo_gpio_basic_api), bmc_extrst_ctrl_out_gpios)
void pfr_bmc_extrst_enable_ctrl(bool enable)
{
	int ret;
	const struct gpio_dt_spec gpio_h2 =
		GPIO_DT_SPEC_GET_BY_IDX(DT_INST(0, demo_gpio_basic_api),
						bmc_extrst_ctrl_out_gpios, 0);

	if (enable)
		gpio_pin_set(gpio_h2.port, gpio_h2.pin, 0);
	else
		gpio_pin_set(gpio_h2.port, gpio_h2.pin, 1);

	ret = gpio_pin_configure_dt(&gpio_h2, GPIO_OUTPUT);
	if (ret)
		return;

	k_busy_wait(10000); /* 10ms */
}
#endif

#if DT_NODE_HAS_PROP(DT_INST(0, demo_gpio_basic_api), pch_rst_ctrl_out_gpios)
void pfr_pch_rst_enable_ctrl(bool enable)
{
	int ret;
	const struct gpio_dt_spec gpio_m2 =
		GPIO_DT_SPEC_GET_BY_IDX(DT_INST(0, demo_gpio_basic_api),
						pch_rst_ctrl_out_gpios, 0);

	if (enable)
		gpio_pin_set(gpio_m2.port, gpio_m2.pin, 0);
	else
		gpio_pin_set(gpio_m2.port, gpio_m2.pin, 1);

	ret = gpio_pin_configure_dt(&gpio_m2, GPIO_OUTPUT);
	if (ret)
		return;

	k_busy_wait(10000); /* 10ms */
}
#endif

