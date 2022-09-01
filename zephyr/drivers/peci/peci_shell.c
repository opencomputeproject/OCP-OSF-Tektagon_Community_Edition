/*
 * Copyright (c) 2021 ASPEED
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief PECI shell commands.
 */

#include <shell/shell.h>
#include <drivers/peci.h>
#include <stdlib.h>

static int cmd_init(const struct shell *shell, size_t argc, char **argv)
{
	const struct device *dev;
	int ret;
	uint32_t bitrate;

	dev = device_get_binding(argv[1]);
	if (!dev) {
		shell_error(shell, "peci device not found");
		return -EINVAL;
	}

	bitrate = strtoul(argv[2], NULL, 0);

	ret = peci_config(dev, bitrate);
	if (ret) {
		printk("set bitrate %dKbps failed %d\n", bitrate, ret);
		return ret;
	}
	ret = peci_enable(dev);
	if (ret) {
		printk("peci enable failed %d\n", ret);
		return ret;
	}

	return 0;
}

static int cmd_ping(const struct shell *shell, size_t argc, char **argv)
{
	const struct device *dev;
	struct peci_msg packet;
	int ret;

	dev = device_get_binding(argv[1]);
	if (!dev) {
		shell_error(shell, "peci device not found");
		return -EINVAL;
	}

	packet.addr = strtoul(argv[2], NULL, 0);
	packet.cmd_code = PECI_CMD_PING;
	packet.tx_buffer.buf = NULL;
	packet.tx_buffer.len = PECI_PING_WR_LEN;
	packet.rx_buffer.buf = NULL;
	packet.rx_buffer.len = PECI_PING_RD_LEN;

	ret = peci_transfer(dev, &packet);
	if (ret) {
		printk("ping failed %d\n", ret);
		return ret;
	}
	printk("Success\n");
	return 0;
}

static int cmd_getdib(const struct shell *shell, size_t argc, char **argv)
{
	const struct device *dev;
	struct peci_msg packet;
	uint32_t index;
	int ret;
	uint8_t peci_resp_buf[PECI_GET_DIB_RD_LEN + 1];

	dev = device_get_binding(argv[1]);
	if (!dev) {
		shell_error(shell, "peci device not found");
		return -EINVAL;
	}

	packet.addr = strtoul(argv[2], NULL, 0);
	packet.cmd_code = PECI_CMD_GET_DIB;
	packet.tx_buffer.buf = NULL;
	packet.tx_buffer.len = PECI_GET_DIB_WR_LEN;
	packet.rx_buffer.buf = peci_resp_buf;
	packet.rx_buffer.len = PECI_GET_DIB_RD_LEN;

	ret = peci_transfer(dev, &packet);
	if (ret) {
		printk("getdib failed %d\n", ret);
		return ret;
	}
	for (index = 0; index < PECI_GET_DIB_RD_LEN; index++)
		printk("%02x ", peci_resp_buf[index]);
	printk("\n");
	return 0;
}

static int cmd_gettemp(const struct shell *shell, size_t argc, char **argv)
{
	const struct device *dev;
	struct peci_msg packet;
	uint32_t index;
	int ret;
	uint8_t peci_resp_buf[PECI_GET_TEMP_RD_LEN + 1];

	dev = device_get_binding(argv[1]);
	if (!dev) {
		shell_error(shell, "peci device not found");
		return -EINVAL;
	}

	packet.addr = strtoul(argv[2], NULL, 0);
	packet.cmd_code = PECI_CMD_GET_TEMP0;
	packet.tx_buffer.buf = NULL;
	packet.tx_buffer.len = PECI_GET_TEMP_WR_LEN;
	packet.rx_buffer.buf = peci_resp_buf;
	packet.rx_buffer.len = PECI_GET_TEMP_RD_LEN;

	ret = peci_transfer(dev, &packet);
	if (ret) {
		printk("gettemp failed %d\n", ret);
		return ret;
	}
	for (index = 0; index < PECI_GET_TEMP_RD_LEN; index++)
		printk("%02x ", peci_resp_buf[index]);
	printk("\n");
	return 0;
}

static int cmd_raw(const struct shell *shell, size_t argc, char **argv)
{
	const struct device *dev;
	struct peci_msg packet;
	uint32_t index;
	int ret, argv_index;
	uint8_t peci_req_buf[64];
	uint8_t peci_resp_buf[64];

	dev = device_get_binding(argv[1]);
	if (!dev) {
		shell_error(shell, "peci device not found");
		return -EINVAL;
	}

	packet.addr = strtoul(argv[2], NULL, 0);
	packet.cmd_code = strtoul(argv[5], NULL, 16);
	packet.tx_buffer.buf = peci_req_buf;
	packet.tx_buffer.len = strtoul(argv[3], NULL, 0);
	packet.rx_buffer.buf = peci_resp_buf;
	packet.rx_buffer.len = strtoul(argv[4], NULL, 0);

	for (argv_index = 6; argv_index < argc; argv_index++) {
		peci_req_buf[argv_index - 6] = strtoul(argv[argv_index], NULL, 0);
	}

	ret = peci_transfer(dev, &packet);
	if (ret) {
		printk("Transfer failed %d\n", ret);
		return ret;
	}
	for (index = 0; index < packet.rx_buffer.len; index++)
		printk("%02x ", peci_resp_buf[index]);
	printk("\n");
	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(
	peci_cmds,
	SHELL_CMD_ARG(init, NULL, "<device> <kbps>", cmd_init, 3,
		      0),
	SHELL_CMD_ARG(ping, NULL, "<device> <addr>", cmd_ping, 3,
		      0),
	SHELL_CMD_ARG(getdib, NULL, "<device> <addr>", cmd_getdib, 3,
		      0),
	SHELL_CMD_ARG(gettemp, NULL, "<device> <addr>", cmd_gettemp, 3,
		      0),
	SHELL_CMD_ARG(raw, NULL, "<device> <addr> <wr_len> <rd_len> <command(hex)> <...>", cmd_raw, 6,
		      64),
	SHELL_SUBCMD_SET_END);

SHELL_CMD_REGISTER(peci, &peci_cmds, "PECI shell commands", NULL);
