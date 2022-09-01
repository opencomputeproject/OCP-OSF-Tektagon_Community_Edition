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

static const struct device *spim_device;

static int probe_parse_helper(const struct shell *shell, size_t *argc,
		char **argv[], const struct device **spim_dev)
{
	*spim_dev = device_get_binding((*argv)[1]);
	if (!*spim_dev) {
		shell_error(shell, "SPI monitor device/driver is not found!");
		return -ENODEV;
	}

	return 0;
}

static int cmd_parse_helper(const struct shell *shell, size_t *argc,
		char **argv[], uint8_t *cmd)
{
	char *endptr;

	if (*argc < 2) {
		shell_error(shell, "Missing command.");
		return -EINVAL;
	}

	*cmd = strtoul((*argv)[1], &endptr, 16);

	return 0;
}

static int addr_parse_helper(const struct shell *shell, size_t *argc,
		char **argv[], bool *enable, mm_reg_t *addr, uint32_t *len)
{
	char *endptr;

	if (*argc < 4) {
		shell_error(shell, "Missing address or length parameter.");
		return -EINVAL;
	}

	*enable = false;
	if (strncmp((*argv)[1], "enable", 6) == 0)
		*enable = true;

	*addr = strtoul((*argv)[2], &endptr, 16);
	*len = strtoul((*argv)[3], &endptr, 16);

	return 0;
}

static int cmd_probe(const struct shell *shell, size_t argc, char *argv[])
{
	int ret;

	ret = probe_parse_helper(shell, &argc, &argv, &spim_device);
	if (ret)
		goto end;

	shell_print(shell, "SPI monitor device, %s, is found!", spim_device->name);

end:
	return ret;
}

static int dump_allow_cmd_table(const struct shell *shell, size_t argc, char *argv[])
{
	if (spim_device == NULL) {
		shell_error(shell, "Please set the device first.");
		return -ENODEV;
	}

	spim_dump_allow_command_table(spim_device);

	return 0;
}

static int add_allow_cmd(const struct shell *shell, size_t argc, char *argv[])
{
	int ret;
	uint8_t cmd = 0;

	if (spim_device == NULL) {
		shell_error(shell, "Please set the device first.");
		return -ENODEV;
	}

	ret = cmd_parse_helper(shell, &argc, &argv, &cmd);
	if (ret)
		goto end;

	if (argc == 3 && strncmp(argv[2], "once", 4) == 0)
		ret = spim_add_allow_command(spim_device, cmd, FLAG_CMD_TABLE_VALID_ONCE);
	else
		ret = spim_add_allow_command(spim_device, cmd, FLAG_CMD_TABLE_VALID);

end:
	return ret;
}

static int remove_allow_cmd(const struct shell *shell, size_t argc, char *argv[])
{
	int ret;
	uint8_t cmd = 0;

	if (spim_device == NULL) {
		shell_error(shell, "Please set the device first.");
		return -ENODEV;
	}

	ret = cmd_parse_helper(shell, &argc, &argv, &cmd);
	if (ret)
		goto end;

	ret = spim_remove_allow_command(spim_device, cmd);
	if (ret)
		goto end;

end:
	return ret;
}

static int lock_allow_cmd(const struct shell *shell, size_t argc, char *argv[])
{
	int ret;
	uint8_t cmd = 0;

	if (spim_device == NULL) {
		shell_error(shell, "Please set the device first.");
		return -ENODEV;
	}

	if (strncmp(argv[1], "all", 3) == 0) {
		/* lock individual register */
		ret = spim_lock_allow_command_table(spim_device, 0, FLAG_CMD_TABLE_LOCK_ALL);
		goto end;
	}

	ret = cmd_parse_helper(shell, &argc, &argv, &cmd);
	if (ret)
		goto end;

	/* lock individual register */
	ret = spim_lock_allow_command_table(spim_device, cmd, 0);

end:
	return ret;
}

static int dump_rw_addr_priv_table(const struct shell *shell, size_t argc, char *argv[])
{
	if (spim_device == NULL) {
		shell_error(shell, "Please set the device first.");
		return -ENODEV;
	}

	spim_dump_rw_addr_privilege_table(spim_device);

	return 0;
}

static int read_addr_priv_table_config(const struct shell *shell, size_t argc, char *argv[])
{
	int ret;
	mm_reg_t addr = 0;
	uint32_t len = 0;
	bool enable = false;
	enum addr_priv_op op;

	if (spim_device == NULL) {
		shell_error(shell, "Please set the device first.");
		return -ENODEV;
	}

	ret = addr_parse_helper(shell, &argc, &argv, &enable, &addr, &len);
	if (ret)
		goto end;

	printk("read: %s, addr: 0x%08lx, len: 0x%08x\n",
		enable ? "enable" : "disable", addr, len);

	if (enable)
		op = FLAG_ADDR_PRIV_ENABLE;
	else
		op = FLAG_ADDR_PRIV_DISABLE;

	ret = spim_address_privilege_config(
		spim_device, FLAG_ADDR_PRIV_READ_SELECT, op, addr, len);

end:
	return ret;
}

static int write_addr_priv_table_config(const struct shell *shell, size_t argc, char *argv[])
{
	int ret;
	mm_reg_t addr = 0;
	uint32_t len = 0;
	bool enable = false;
	enum addr_priv_op op;

	if (spim_device == NULL) {
		shell_error(shell, "Please set the device first.");
		return -ENODEV;
	}

	ret = addr_parse_helper(shell, &argc, &argv, &enable, &addr, &len);
	if (ret)
		goto end;

	printk("write: %s, addr: 0x%08lx, len: 0x%08x\n",
		enable ? "enable" : "disable", addr, len);

	if (enable)
		op = FLAG_ADDR_PRIV_ENABLE;
	else
		op = FLAG_ADDR_PRIV_DISABLE;

	ret = spim_address_privilege_config(
		spim_device, FLAG_ADDR_PRIV_WRITE_SELECT, op, addr, len);

end:
	return ret;
}

static int lock_addr_priv_table(const struct shell *shell, size_t argc, char *argv[])
{
	int ret;

	if (spim_device == NULL) {
		shell_error(shell, "Please set the device first.");
		return -ENODEV;
	}

	if (strncmp(argv[1], "read", 4) == 0) {
		spim_lock_rw_privilege_table(spim_device, FLAG_ADDR_PRIV_READ_SELECT);
	} else if (strncmp(argv[1], "write", 5) == 0) {
		spim_lock_rw_privilege_table(spim_device, FLAG_ADDR_PRIV_WRITE_SELECT);
	} else {
		shell_error(shell, "Invalid command operation.");
		ret = -ENOTSUP;
	}

	return ret;
}

static int cmd_lock(const struct shell *shell, size_t argc, char *argv[])
{

	if (spim_device == NULL) {
		shell_error(shell, "Please set the device first.");
		return -ENODEV;
	}

	spim_lock_common(spim_device);

	return 0;
}

static int spi_monitor_enabled(const struct shell *shell, size_t argc, char *argv[])
{

	if (spim_device == NULL) {
		shell_error(shell, "Please set the device first.");
		return -ENODEV;
	}

	spim_monitor_enable(spim_device, true);

	return 0;
}

static int spi_monitor_disabled(const struct shell *shell, size_t argc, char *argv[])
{

	if (spim_device == NULL) {
		shell_error(shell, "Please set the device first.");
		return -ENODEV;
	}

	spim_monitor_enable(spim_device, false);

	return 0;
}

static int external_mux_config(const struct shell *shell, size_t argc, char *argv[])
{
	uint32_t flag;
	char *endptr;

	if (spim_device == NULL) {
		shell_error(shell, "Please set the device first.");
		return -ENODEV;
	}

	flag = strtoul(argv[1], &endptr, 16);

	if (flag == 0)
		spim_ext_mux_config(spim_device, SPIM_MASTER_MODE);
	else
		spim_ext_mux_config(spim_device, SPIM_MONITOR_MODE);

	return 0;
}

static int passthrough_mode_config(const struct shell *shell, size_t argc, char *argv[])
{
	uint32_t flag;
	char *endptr;
	enum spim_passthrough_mode mode = SPIM_SINGLE_PASSTHROUGH;

	if (spim_device == NULL) {
		shell_error(shell, "Please set the device first.");
		return -ENODEV;
	}

	if (argc < 3) {
		shell_error(shell, "spim config passthrough <multi/single> <0/1>.");
		return -EINVAL;
	}

	if (strncmp(argv[1], "multi", 5) == 0)
		mode = SPIM_MULTI_PASSTHROUGH;

	flag = strtoul(argv[2], &endptr, 16);

	if (flag == 0)
		spim_passthrough_config(spim_device, mode, false);
	else
		spim_passthrough_config(spim_device, mode, true);

	return 0;
}

static int spim_spi_ctrl_detour_config(
	const struct shell *shell, size_t argc, char *argv[])
{
	uint32_t enable;
	char *endptr;
	enum spim_spi_master spi = SPI1;

	if (spim_device == NULL) {
		shell_error(shell, "Please set the device first.");
		return -ENODEV;
	}

	if (argc < 3) {
		shell_error(shell, "spim config detour <spi master> <0/1>.");
		return -EINVAL;
	}

	if (strncmp(argv[1], "spi1", 4) == 0)
		spi = SPI1;
	else if (strncmp(argv[1], "spi2", 4) == 0)
		spi = SPI2;

	enable = strtoul(argv[2], &endptr, 16);

	if (enable == 0)
		spim_spi_ctrl_detour_enable(spim_device, spi, false);
	else
		spim_spi_ctrl_detour_enable(spim_device, spi, true);

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(sub_spim_cmds,
	SHELL_CMD_ARG(dump, NULL, "\"dump\"", dump_allow_cmd_table, 1, 0),
	SHELL_CMD_ARG(add, NULL, "<command>", add_allow_cmd, 2, 1),
	SHELL_CMD_ARG(rm, NULL, "<command>", remove_allow_cmd, 2, 0),
	SHELL_CMD_ARG(lock, NULL, "<command>", lock_allow_cmd, 2, 0),

	SHELL_SUBCMD_SET_END
);

SHELL_STATIC_SUBCMD_SET_CREATE(sub_spim_addr,
	SHELL_CMD_ARG(dump, NULL, "\"dump\"", dump_rw_addr_priv_table, 1, 0),
	SHELL_CMD_ARG(read, NULL, "<enable/disable> <addr> <len>",
		read_addr_priv_table_config, 4, 0),
	SHELL_CMD_ARG(write, NULL, "<enable/disable> <addr> <len>",
		write_addr_priv_table_config, 4, 0),
	SHELL_CMD_ARG(lock, NULL, "<read/write>", lock_addr_priv_table, 2, 0),

	SHELL_SUBCMD_SET_END
);

SHELL_STATIC_SUBCMD_SET_CREATE(sub_spim_config,
	SHELL_CMD_ARG(enable, NULL, "\"enable\"", spi_monitor_enabled, 1, 0),
	SHELL_CMD_ARG(disable, NULL, "\"enable\"", spi_monitor_disabled, 1, 0),
	SHELL_CMD_ARG(extmux, NULL, "<0/1> for clear/set", external_mux_config, 2, 0),
	SHELL_CMD_ARG(passthrough, NULL, "<multi/single> <0/1> for clear/set",
		passthrough_mode_config, 3, 0),
	SHELL_CMD_ARG(detour, NULL, "<SPI master> <0/1>",
		spim_spi_ctrl_detour_config, 3, 0),

	SHELL_SUBCMD_SET_END
);

SHELL_STATIC_SUBCMD_SET_CREATE(spim_cmds,
	SHELL_CMD_ARG(set_dev, NULL, "<device>", cmd_probe, 2, 0),
	SHELL_CMD(cmd, &sub_spim_cmds, "cmd table related operations", NULL),
	SHELL_CMD(addr, &sub_spim_addr, "address privilege table related operations", NULL),
	SHELL_CMD_ARG(lock, NULL, "lock", cmd_lock, 1, 0),
	SHELL_CMD(config, &sub_spim_config, "SPI monitor configuration", NULL),

	SHELL_SUBCMD_SET_END
);

SHELL_CMD_REGISTER(spim, &spim_cmds, "SPI monitor shell commands", NULL);

