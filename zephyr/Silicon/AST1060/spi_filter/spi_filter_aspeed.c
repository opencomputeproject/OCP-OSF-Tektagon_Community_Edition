/*
 * Copyright (c) 2021 AMI
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <drivers/flash.h>
#include <drivers/spi_nor.h>
#include <spi_filter/spi_filter_aspeed.h>
#include <kernel.h>
#include <sys/util.h>
#include <stdlib.h>
#include <string.h>
#include <zephyr.h>




void SPI_Monitor_Enable(char *dev_name, bool enabled)
{
	const struct device *dev_m = NULL;

	dev_m = device_get_binding(dev_name);
	spim_rst_flash(dev_m, 1000);
	spim_passthrough_config(dev_m, 0, false);
	spim_ext_mux_config(dev_m, 1);
	spim_monitor_enable(dev_m, enabled);
}

int Set_SPI_Filter_RW_Region(char *dev_name, enum addr_priv_rw_select rw_select, enum addr_priv_op op, mm_reg_t addr, uint32_t len)
{
	int ret = 0;
	const struct device *dev_m = NULL;

	dev_m = device_get_binding(dev_name);

	ret = spim_address_privilege_config(dev_m, rw_select, op, addr, len);

	return ret;

}