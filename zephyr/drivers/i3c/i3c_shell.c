/*
 * Copyright (c) 2021 ASPEED Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <shell/shell.h>
#include <ctype.h>
#include <stdlib.h>
#include <device.h>
#include <drivers/i3c/i3c.h>

#define I3C_DEVICE_PREFIX		"I3C_"
#define I3C_SHELL_MAX_XFER_NUM		2
#define I3C_SHELL_MAX_BUF_SIZE		16
#define I3C_SHELL_MAX_DESC_NUM		8

static uint8_t data_buf[I3C_SHELL_MAX_XFER_NUM][I3C_SHELL_MAX_BUF_SIZE];
static int i3c_shell_num_of_descs;
static struct i3c_dev_desc i3c_shell_desc_tbl[I3C_SHELL_MAX_DESC_NUM];

static void device_name_get(size_t idx, struct shell_static_entry *entry)
{
	const struct device *dev = shell_device_lookup(idx, I3C_DEVICE_PREFIX);

	entry->syntax = (dev != NULL) ? dev->name : NULL;
	entry->handler = NULL;
	entry->help = NULL;
	entry->subcmd = NULL;
}
SHELL_DYNAMIC_CMD_CREATE(dsub_device_name, device_name_get);

static struct i3c_dev_desc *find_matching_desc(const struct device *dev, uint8_t desc_addr)
{
	struct i3c_dev_desc *desc = NULL;
	int i;

	for (i = 0; i < I3C_SHELL_MAX_DESC_NUM; i++) {
		desc = &i3c_shell_desc_tbl[i];
		if (desc->master_dev == dev && desc->info.dynamic_addr == desc_addr) {
			break;
		}
	}

	return desc;
}

static uint32_t args_to_wdata(char *arg, uint8_t *buf)
{
	char *data_ptrs[I3C_SHELL_MAX_BUF_SIZE];
	char *state;
	int i = 0, len = 0;

	data_ptrs[i] = strtok_r(arg, ",", &state);
	while (data_ptrs[i] && i < I3C_SHELL_MAX_BUF_SIZE - 1) {
		data_ptrs[++i] = strtok_r(NULL, ",", &state);
	}

	for (len = 0; len < i; len++) {
		buf[len] = strtoul(data_ptrs[len], NULL, 0);
	}

	return len;
}

static const char priv_xfer_helper[] = "i3c xfer <dev> -a <addr> -w <wdata> -r <read length>";
static int cmd_priv_xfer(const struct shell *shell, size_t argc, char **argv)
{
	const struct device *dev;
	struct i3c_priv_xfer xfers[I3C_SHELL_MAX_XFER_NUM];
	struct i3c_dev_desc *desc;
	struct getopt_state *state;
	int nxfers = 0;
	int addr = -1;
	int c, ret;

	dev = device_get_binding(argv[1]);
	if (!dev) {
		shell_error(shell, "I3C: Device driver %s not found.", argv[1]);
		return -ENODEV;
	}

	while ((c = shell_getopt(shell, argc - 1, &argv[1], "ha:w:r:")) != -1) {
		state = shell_getopt_state_get(shell);
		switch (c) {
		case 'a':
			addr = strtoul(state->optarg, NULL, 0);
			break;
		case 'w':
			xfers[nxfers].rnw = 0;
			xfers[nxfers].data.out = data_buf[nxfers];
			xfers[nxfers].len = args_to_wdata(state->optarg, data_buf[nxfers]);
			nxfers++;
			break;
		case 'r':
			xfers[nxfers].rnw = 1;
			xfers[nxfers].data.in = data_buf[nxfers];
			xfers[nxfers].len = atoi(state->optarg);
			nxfers++;
			break;
		case 'h':
			shell_help(shell);
			return SHELL_CMD_HELP_PRINTED;
		case '?':
			if ((state->optopt == 'a') || (state->optopt == 'w') ||
			    (state->optopt == 'r')) {
				shell_print(shell, "Option -%c requires an argument.",
					    state->optopt);
			} else if (isprint(state->optopt)) {
				shell_print(shell, "Unknown option `-%c'.", state->optopt);
			} else {
				shell_print(shell, "Unknown option character `\\x%x'.",
					    state->optopt);
			}
			return 1;
		default:
			break;
		}
	}

	if (addr < 0) {
		shell_error(shell, "I3C: Slave not assigned.");
		return -ENODEV;
	}

	desc = find_matching_desc(dev, addr);
	if (!desc) {
		shell_error(shell, "I3C: Slave %x not found.", addr);
		return -ENODEV;
	}

	shell_print(shell, "Private transfer to address 0x%02x\n", addr);
	ret = i3c_master_priv_xfer(desc, xfers, nxfers);
	if (ret) {
		shell_print(shell, "Failed to private transfer: %d\n", ret);
	}

	for (int i = 0; i < nxfers; i++) {
		if (xfers[i].rnw) {
			shell_hexdump(shell, xfers[i].data.out, xfers[i].len);
		}
	}

	return ret;
}

static const char send_ccc_helper[] = "i3c ccc <dev> -a <addr> -i <id> -w <wdata> -r <read length>";
static int cmd_send_ccc(const struct shell *shell, size_t argc, char **argv)
{
	const struct device *dev;
	struct i3c_ccc_cmd ccc;
	struct getopt_state *state;
	int c, ret;

	dev = device_get_binding(argv[1]);
	if (!dev) {
		shell_error(shell, "I3C: Device driver %s not found.", argv[1]);
		return -ENODEV;
	}

	ccc.rnw = 0;

	while ((c = shell_getopt(shell, argc - 1, &argv[1], "ha:i:w:r:")) != -1) {
		state = shell_getopt_state_get(shell);
		switch (c) {
		case 'a':
			ccc.addr = strtoul(state->optarg, NULL, 0);
			break;
		case 'i':
			ccc.id = strtoul(state->optarg, NULL, 0);
			break;
		case 'w':
			ccc.rnw = 0;
			ccc.payload.data = data_buf[0];
			ccc.payload.length = args_to_wdata(state->optarg, data_buf[0]);
			break;
		case 'r':
			ccc.rnw = 1;
			ccc.payload.data = data_buf[0];
			ccc.payload.length = atoi(state->optarg);
			break;
		case 'h':
			shell_help(shell);
			return SHELL_CMD_HELP_PRINTED;
		case '?':
			if ((state->optopt == 'a') || (state->optopt == 'w') ||
			    (state->optopt == 'r') || (state->optopt == 'i')) {
				shell_print(shell, "Option -%c requires an argument.",
					    state->optopt);
			} else if (isprint(state->optopt)) {
				shell_print(shell, "Unknown option `-%c'.", state->optopt);
			} else {
				shell_print(shell, "Unknown option character `\\x%x'.",
					    state->optopt);
			}
			return 1;
		default:
			break;
		}
	}

	ccc.ret = 0;
	if (ccc.addr != I3C_BROADCAST_ADDR) {
		ccc.id |= I3C_CCC_DIRECT;
	}

	shell_print(shell, "Send CCC ID 0x%02x (%s) to address 0x%02x\n", ccc.id,
		    ccc.rnw ? "r" : "w", ccc.addr);
	ret = i3c_master_send_ccc(dev, &ccc);
	if (ret) {
		shell_print(shell, "Failed to send ccc: %d\n", ret);
	}

	if (ccc.rnw) {
		shell_hexdump(shell, data_buf[0], ccc.payload.length);
	}

	return ret;
}

static const char attach_helper[] = "i3c attach <dev> -a <addr> -m <i2c mode>";
static int cmd_attach(const struct shell *shell, size_t argc, char **argv)
{
	const struct device *dev;
	struct i3c_dev_desc *desc;
	struct getopt_state *state;
	int c, ret;

	dev = device_get_binding(argv[1]);
	if (!dev) {
		shell_error(shell, "I3C: Device driver %s not found.", argv[1]);
		return -ENODEV;
	}

	desc = &i3c_shell_desc_tbl[i3c_shell_num_of_descs];
	desc->info.i2c_mode = 0;

	while ((c = shell_getopt(shell, argc - 1, &argv[1], "ha:m:")) != -1) {
		state = shell_getopt_state_get(shell);
		switch (c) {
		case 'a':
			desc->info.assigned_dynamic_addr = strtoul(state->optarg, NULL, 0);
			desc->info.static_addr = desc->info.assigned_dynamic_addr;
			break;
		case 'm':
			desc->info.i2c_mode = strtoul(state->optarg, NULL, 0);
			break;
		case 'h':
			shell_help(shell);
			return SHELL_CMD_HELP_PRINTED;
		case '?':
			if ((state->optopt == 'a') || (state->optopt == 'm')) {
				shell_print(shell, "Option -%c requires an argument.",
					    state->optopt);
			} else if (isprint(state->optopt)) {
				shell_print(shell, "Unknown option `-%c'.", state->optopt);
			} else {
				shell_print(shell, "Unknown option character `\\x%x'.",
					    state->optopt);
			}
			return 1;
		default:
			break;
		}
	}

	shell_print(shell, "Attach address 0x%02x to %s\n", desc->info.assigned_dynamic_addr,
		    dev->name);
	ret = i3c_master_attach_device(dev, desc);
	if (ret) {
		shell_print(shell, "Failed to attach device: %d\n", ret);
	} else {
		i3c_shell_num_of_descs++;
	}

	return ret;
}


#ifdef CONFIG_I3C_SLAVE_MQUEUE
int i3c_slave_mqueue_read(const struct device *dev, uint8_t *dest, int budget);
int i3c_slave_mqueue_write(const struct device *dev, uint8_t *src, int size);

static const char smq_xfer_helper[] = "i3c smq <dev> -w <wdata> -r <read length>";
static int cmd_smq_xfer(const struct shell *shell, size_t argc, char **argv)
{
	const struct device *dev;
	struct getopt_state *state;
	int c, len, ret;

	dev = device_get_binding(argv[1]);
	if (!dev) {
		shell_error(shell, "I3C: Device driver %s not found.", argv[1]);
		return -ENODEV;
	}

	while ((c = shell_getopt(shell, argc - 1, &argv[1], "w:r:h")) != -1) {
		state = shell_getopt_state_get(shell);
		switch (c) {
		case 'w':
			len = args_to_wdata(state->optarg, data_buf[0]);
			ret = i3c_slave_mqueue_write(dev, data_buf[0], len);
			return 0;
		case 'r':
			len = strtoul(state->optarg, NULL, 0);
			i3c_slave_mqueue_read(dev, data_buf[0], len);
			shell_hexdump(shell, data_buf[0], len);
			return 0;
		case 'h':
			shell_help(shell);
			return SHELL_CMD_HELP_PRINTED;
		case '?':
			if ((state->optopt == 'r') || (state->optopt == 'w')) {
				shell_print(shell, "Option -%c requires an argument.",
					    state->optopt);
			} else if (isprint(state->optopt)) {
				shell_print(shell, "Unknown option `-%c'.", state->optopt);
			} else {
				shell_print(shell, "Unknown option character `\\x%x'.",
					    state->optopt);
			}
			return 1;
		default:
			break;
		}
	}

	return 0;
}
#endif

SHELL_STATIC_SUBCMD_SET_CREATE(sub_i3c_cmds,
	SHELL_CMD(attach, &dsub_device_name, attach_helper, cmd_attach),
	SHELL_CMD(ccc, &dsub_device_name, send_ccc_helper, cmd_send_ccc),
	SHELL_CMD(xfer, &dsub_device_name, priv_xfer_helper, cmd_priv_xfer),
#ifdef CONFIG_I3C_SLAVE_MQUEUE
	SHELL_CMD(smq, &dsub_device_name, smq_xfer_helper, cmd_smq_xfer),
#endif
	SHELL_SUBCMD_SET_END);
SHELL_CMD_REGISTER(i3c, &sub_i3c_cmds, "I3C commands", NULL);
