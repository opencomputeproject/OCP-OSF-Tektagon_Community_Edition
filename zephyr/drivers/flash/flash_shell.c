/*
 * Copyright (c) 2017 Nordic Semiconductor ASA
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>

#include <shell/shell.h>
#include <sys/util.h>

#include <stdlib.h>
#include <string.h>
#include <drivers/flash.h>
#include <drivers/spi_nor.h>
#include <soc.h>

#include <kernel.h>

#define BUF_ARRAY_CNT 16
#define TEST_ARR_SIZE 0x1000

#ifdef DT_CHOSEN_ZEPHYR_FLASH_CONTROLLER_LABEL
#define FLASH_DEV_NAME DT_CHOSEN_ZEPHYR_FLASH_CONTROLLER_LABEL
#else
#define FLASH_DEV_NAME ""
#endif

static uint8_t test_arr[TEST_ARR_SIZE] NON_CACHED_BSS_ALIGN16;
static uint8_t read_back_arr[TEST_ARR_SIZE] NON_CACHED_BSS_ALIGN16;
static uint8_t op_arr[TEST_ARR_SIZE] NON_CACHED_BSS_ALIGN16;

static int parse_helper(const struct shell *shell, size_t *argc,
		char **argv[], const struct device * *flash_dev,
		uint32_t *addr)
{
	char *endptr;

	*addr = strtoul((*argv)[1], &endptr, 16);
	*flash_dev = device_get_binding((*endptr != '\0') ? (*argv)[1] :
			FLASH_DEV_NAME);
	if (!*flash_dev) {
		shell_error(shell, "Flash driver was not found!");
		return -ENODEV;
	}
	if (*endptr == '\0') {
		return 0;
	}
	if (*argc < 3) {
		shell_error(shell, "Missing address.");
		return -EINVAL;
	}
	*addr = strtoul((*argv)[2], &endptr, 16);
	(*argc)--;
	(*argv)++;
	return 0;
}

static int cmd_erase(const struct shell *shell, size_t argc, char *argv[])
{
	const struct device *flash_dev;
	uint32_t page_addr;
	int result;
	uint32_t size;

	result = parse_helper(shell, &argc, &argv, &flash_dev, &page_addr);
	if (result) {
		return result;
	}
	if (argc > 2) {
		size = strtoul(argv[2], NULL, 16);
	} else {
		struct flash_pages_info info;

		result = flash_get_page_info_by_offs(flash_dev, page_addr,
						     &info);

		if (result != 0) {
			shell_error(shell, "Could not determine page size, "
				    "code %d.", result);
			return -EINVAL;
		}

		size = info.size;
	}

	result = flash_erase(flash_dev, page_addr, size);

	if (result) {
		shell_error(shell, "Erase Failed, code %d.", result);
	} else {
		shell_print(shell, "Erase success.");
	}

	return result;
}

static int cmd_write(const struct shell *shell, size_t argc, char *argv[])
{
	uint32_t check_array[BUF_ARRAY_CNT];
	uint32_t buf_array[BUF_ARRAY_CNT];
	const struct device *flash_dev;
	uint32_t w_addr;
	int ret;
	int j = 0;

	ret = parse_helper(shell, &argc, &argv, &flash_dev, &w_addr);
	if (ret) {
		return ret;
	}

	if (argc <= 2) {
		shell_error(shell, "Missing data to be written.");
		return -EINVAL;
	}

	for (int i = 2; i < argc && i < BUF_ARRAY_CNT; i++) {
		buf_array[j] = strtoul(argv[i], NULL, 16);
		check_array[j] = ~buf_array[j];
		j++;
	}

	if (flash_write(flash_dev, w_addr, buf_array,
			sizeof(buf_array[0]) * j) != 0) {
		shell_error(shell, "Write internal ERROR!");
		return -EIO;
	}

	shell_print(shell, "Write OK.");

	flash_read(flash_dev, w_addr, check_array, sizeof(buf_array[0]) * j);

	if (memcmp(buf_array, check_array, sizeof(buf_array[0]) * j) == 0) {
		shell_print(shell, "Verified.");
	} else {
		shell_error(shell, "Verification ERROR!");
		return -EIO;
	}

	return 0;
}

static int cmd_read(const struct shell *shell, size_t argc, char *argv[])
{
	const struct device *flash_dev;
	uint32_t addr;
	int todo;
	int upto;
	int cnt;
	int ret;

	ret = parse_helper(shell, &argc, &argv, &flash_dev, &addr);
	if (ret) {
		return ret;
	}

	if (argc > 2) {
		cnt = strtoul(argv[2], NULL, 16);
	} else {
		cnt = 1;
	}

	for (upto = 0; upto < cnt; upto += todo) {
		uint8_t data[SHELL_HEXDUMP_BYTES_IN_LINE];

		todo = MIN(cnt - upto, SHELL_HEXDUMP_BYTES_IN_LINE);
		ret = flash_read(flash_dev, addr, data, todo);
		if (ret != 0) {
			shell_error(shell, "Read ERROR!");
			return -EIO;
		}
		shell_hexdump_line(shell, addr, data, todo);
		addr += todo;
	}

	shell_print(shell, "");

	return 0;
}

static int cmd_test(const struct shell *shell, size_t argc, char *argv[])
{
	const struct device *flash_dev;
	uint32_t repeat;
	int result;
	uint32_t addr;
	uint32_t size;

	result = parse_helper(shell, &argc, &argv, &flash_dev, &addr);
	if (result) {
		return result;
	}

	size = strtoul(argv[2], NULL, 16);
	repeat = strtoul(argv[3], NULL, 16);
	if (size > TEST_ARR_SIZE) {
		shell_error(shell, "<size> must be at most 0x%x.",
			    TEST_ARR_SIZE);
		return -EINVAL;
	}

	for (uint32_t i = 0; i < size; i++) {
		test_arr[i] = (uint8_t)i;
	}

	result = 0;

	while (repeat--) {
		result = flash_erase(flash_dev, addr, size);

		if (result) {
			shell_error(shell, "Erase Failed, code %d.", result);
			break;
		}

		shell_print(shell, "Erase OK.");

		result = flash_write(flash_dev, addr, test_arr, size);

		if (result) {
			shell_error(shell, "Write internal ERROR!");
			break;
		}

		shell_print(shell, "Write OK.");
	}

	if (result == 0) {
		shell_print(shell, "Erase-Write test done.");
	}

	return result;
}

static int do_erase_write_verify(const struct device *flash_device,
			uint32_t op_addr, uint8_t *write_buf, uint8_t *read_back_buf,
			uint32_t erase_sz)
{
	uint32_t ret = 0;
	uint32_t i;

	ret = flash_erase(flash_device, op_addr, erase_sz);
	if (ret != 0) {
		printk("[%s][%d] erase failed at %d.\n",
			__func__, __LINE__, op_addr);
		goto end;
	}

	ret = flash_write(flash_device, op_addr, write_buf, erase_sz);
	if (ret != 0) {
		printk("[%s][%d] write failed at %d.\n",
			__func__, __LINE__, op_addr);
		goto end;
	}

	flash_read(flash_device, op_addr, read_back_buf, erase_sz);
	if (ret != 0) {
		printk("[%s][%d] write failed at %d.\n",
			__func__, __LINE__, op_addr);
		goto end;
	}

	if (memcmp(write_buf, read_back_buf, erase_sz) != 0) {
		ret = -EINVAL;
		printk("ERROR: %s %d fail to write flash at 0x%x\n",
				__func__, __LINE__, op_addr);
		printk("to be written:\n");
		for (i = 0; i < 256; i++) {
			printk("%x ", write_buf[i]);
			if (i % 16 == 15)
				printk("\n");
		}

		printk("readback:\n");
		for (i = 0; i < 256; i++) {
			printk("%x ", read_back_buf[i]);
			if (i % 16 == 15)
				printk("\n");
		}

		goto end;
	}

end:
	return ret;
}

static int do_update(const struct device *flash_device,
				off_t offset, uint8_t *buf, size_t len)
{
	int ret = 0;
	uint32_t flash_sz = flash_get_flash_size(flash_device);
	uint32_t sector_sz = flash_get_write_block_size(flash_device);
	uint32_t flash_offset = (uint32_t)offset;
	uint32_t remain, op_addr = 0, end_sector_addr;
	uint8_t *update_ptr = buf, *op_buf = NULL, *read_back_buf = NULL;
	bool update_it = false;

	printk("Writing %d bytes to %s (offset: 0x%08x)...\n",
			len, flash_device->name, offset);

	if (flash_sz < flash_offset + len) {
		printk("ERROR: update boundary exceeds flash size. (%d, %d, %d)\n",
			flash_sz, flash_offset, len);
		ret = -EINVAL;
		goto end;
	}

	op_buf = (uint8_t *)op_arr;
	read_back_buf = (uint8_t *)read_back_arr;

	/* initial op_addr */
	op_addr = (flash_offset / sector_sz) * sector_sz;

	/* handle the start part which is not multiple of sector size */
	if (flash_offset % sector_sz != 0) {
		ret = flash_read(flash_device, op_addr, op_buf, sector_sz);
		if (ret != 0)
			goto end;

		remain = MIN(sector_sz - (flash_offset % sector_sz), len);
		memcpy((uint8_t *)op_buf + (flash_offset % sector_sz), update_ptr, remain);
		ret = do_erase_write_verify(flash_device, op_addr, op_buf,
								read_back_buf, sector_sz);
		if (ret != 0)
			goto end;

		op_addr += sector_sz;
		update_ptr += remain;
	}

	end_sector_addr = (flash_offset + len) / sector_sz * sector_sz;
	/* handle body */
	for (; op_addr < end_sector_addr;) {
		ret = flash_read(flash_device, op_addr, op_buf, sector_sz);
		if (ret != 0)
			goto end;

		if (memcmp(op_buf, update_ptr, sector_sz) != 0)
			update_it = true;

		if (update_it) {
			ret = do_erase_write_verify(flash_device, op_addr, update_ptr,
								read_back_buf, sector_sz);
			if (ret != 0)
				goto end;
		}

		op_addr += sector_sz;
		update_ptr += sector_sz;
	}

	/* handle remain part */
	if (end_sector_addr < flash_offset + len) {

		ret = flash_read(flash_device, op_addr, op_buf, sector_sz);
		if (ret != 0)
			goto end;

		remain = flash_offset + len - end_sector_addr;
		memcpy((uint8_t *)op_buf, update_ptr, remain);

		ret = do_erase_write_verify(flash_device, op_addr, op_buf,
								read_back_buf, sector_sz);
		if (ret != 0)
			goto end;

		op_addr += remain;
	}

end:
	printk("Update %s.\n", ret ? "FAILED" : "done");

	return ret;
}

#define UPDATE_TEST_PATTERN_SIZE (TEST_ARR_SIZE - 4)

static int cmd_update_test(const struct shell *shell, size_t argc, char *argv[])
{
	int ret = 0;
	const struct device *flash_dev;
	uint32_t addr;
	int cnt;
	static bool test_repeat = true;
	uint32_t i;

	ret = parse_helper(shell, &argc, &argv, &flash_dev, &addr);
	if (ret) {
		return ret;
	}

	if (argc > 2) {
		cnt = strtoul(argv[2], NULL, 16);
	} else {
		cnt = 1;
	}

	while (cnt > 0) {
		if (test_repeat) {
			for (i = 0; i < UPDATE_TEST_PATTERN_SIZE; i++) {
				test_arr[i] = 'a' + (i % 26);
			}
		} else {
			for (i = 0; i < UPDATE_TEST_PATTERN_SIZE; i++) {
				test_arr[i] = 'z' - (i % 26);
			}
		}

		ret = do_update(flash_dev, addr, test_arr, UPDATE_TEST_PATTERN_SIZE);
		if (ret != 0) {
			printk("RW test fail\n");
			break;
		}

		printk("RW test pass\n");

		test_repeat ^= true;
		cnt--;
	}

	return ret;
}

static int cmd_address_mode_config(const struct shell *shell, size_t argc, char *argv[])
{
	int ret = 0;
	const struct device *flash_dev;
	uint32_t addr_mode;

	ret = parse_helper(shell, &argc, &argv, &flash_dev, &addr_mode);
	if (ret)
		return ret;

	if (addr_mode == 4) {
		ret = spi_nor_config_4byte_mode(flash_dev, true);
	} else if (addr_mode == 3) {
		ret = spi_nor_config_4byte_mode(flash_dev, false);
	} else {
		shell_error(shell, "Wrong addrss mode: %d", addr_mode);
		ret = -EINVAL;
	}

	return ret;
}

static void device_name_get(size_t idx, struct shell_static_entry *entry);

SHELL_DYNAMIC_CMD_CREATE(dsub_device_name, device_name_get);

static void device_name_get(size_t idx, struct shell_static_entry *entry)
{
	const struct device *dev = shell_device_lookup(idx, NULL);

	entry->syntax = (dev != NULL) ? dev->name : NULL;
	entry->handler = NULL;
	entry->help  = NULL;
	entry->subcmd = &dsub_device_name;
}

SHELL_STATIC_SUBCMD_SET_CREATE(flash_cmds,
	SHELL_CMD_ARG(erase, &dsub_device_name,
		"[<device>] <page address> [<size>]",
		cmd_erase, 2, 2),
	SHELL_CMD_ARG(read, &dsub_device_name,
		"[<device>] <address> [<Dword count>]",
		cmd_read, 2, 2),
	SHELL_CMD_ARG(test, &dsub_device_name,
		"[<device>] <address> <size> <repeat count>",
		cmd_test, 4, 1),
	SHELL_CMD_ARG(write, &dsub_device_name,
		"[<device>] <address> <dword> [<dword>...]",
		cmd_write, 3, BUF_ARRAY_CNT),
	SHELL_CMD_ARG(update_test, &dsub_device_name,
		"[<device>] <address> [<count>]",
		cmd_update_test, 2, 2),
	SHELL_CMD_ARG(addr_mode, NULL,
		"[<device>] <address byte mode>",
		cmd_address_mode_config, 3, 0),

	SHELL_SUBCMD_SET_END
);

static int cmd_flash(const struct shell *shell, size_t argc, char **argv)
{
	shell_error(shell, "%s:unknown parameter: %s", argv[0], argv[1]);
	return -EINVAL;
}

SHELL_CMD_ARG_REGISTER(flash, &flash_cmds, "Flash shell commands",
		       cmd_flash, 2, 0);
