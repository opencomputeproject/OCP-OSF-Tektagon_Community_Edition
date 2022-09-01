/*
 * Copyright (c) 2021 Chin-Ting Kuo <chin-ting_kuo@aspeedtech.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <shell/shell.h>
#include <sys/util.h>
#include <stdlib.h>
#include <string.h>
#include <drivers/misc/aspeed/pfr_aspeed.h>
#include <soc.h>
#include <kernel.h>

static int bmc_rst_demo(const struct shell *shell, size_t argc, char *argv[])
{

	const struct device *spim_dev1 = NULL;

	spim_dev1 = device_get_binding("spi_m1");
	if (!spim_dev1) {
		shell_error(shell, "cannot get the device, spi_m1.\n");
		return -ENODEV;
	}

	/* SRST# */
	pfr_bmc_rst_enable_ctrl(true);

	spim_rst_flash(spim_dev1, 500);

	pfr_bmc_rst_enable_ctrl(false);

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(pfr_cmds,
	SHELL_CMD_ARG(rst_bmc, NULL, "reset BMC demo", bmc_rst_demo, 1, 0),

	SHELL_SUBCMD_SET_END
);

SHELL_CMD_REGISTER(pfr, &pfr_cmds, "PFR shell commands", NULL);

