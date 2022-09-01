/*
 * Part of this file is merged from flash_shell.c
 *
 * Copyright (c) 2021 ASPEED Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <stdlib.h>
#include <string.h>

#include <zephyr.h>
#include <sys/printk.h>
#include <logging/log.h>
#include <shell/shell.h>
#include <drivers/flash.h>
#include <device.h>
#include <soc.h>
#include <stdlib.h>
#include <sys/util.h>

LOG_MODULE_REGISTER(app);

#define PR_SHELL(shell, fmt, ...)				\
	shell_fprintf(shell, SHELL_NORMAL, fmt, ##__VA_ARGS__)
#define PR_ERROR(shell, fmt, ...)				\
	shell_fprintf(shell, SHELL_ERROR, fmt, ##__VA_ARGS__)
#define PR_INFO(shell, fmt, ...)				\
	shell_fprintf(shell, SHELL_INFO, fmt, ##__VA_ARGS__)
#define PR_WARNING(shell, fmt, ...)				\
	shell_fprintf(shell, SHELL_WARNING, fmt, ##__VA_ARGS__)
/*
 * When DT_CHOSEN_ZEPHYR_FLASH_CONTROLLER_LABEL is available, we use it here.
 * Otherwise the device can be set at runtime with the set_device command.
 */
#ifndef DT_CHOSEN_ZEPHYR_FLASH_CONTROLLER_LABEL
#define DT_CHOSEN_ZEPHYR_FLASH_CONTROLLER_LABEL "fmc_cs0"
#endif

/* Command usage info. */
#define SECTOR_SIZE_HELP \
	("Print the device's sector size.")
#define READ_HELP \
	("<off> <len>\n" \
	 "Read <len> bytes from device offset <off>.")
#define ERASE_HELP \
	("<off> <len>\n\n" \
	 "Erase <len> bytes from device offset <off>, " \
	 "subject to hardware page limitations.")
#define WRITE_HELP \
	("<off> <byte1> [... byteN]\n\n" \
	 "Write given bytes, starting at device offset <off>.\n" \
	 "Pages must be erased before they can be written.")
#define WRITE_UNALIGNED_HELP \
	("<off> <byte1> [... byteN]\n\n" \
	 "Write given bytes, starting at device offset <off>.\n" \
	 "Being unaligned, affected memory areas are backed up, erased, protected" \
	 " and then overwritten.\n" \
	 "This command is designed to test writing to large flash pages.")
#define WRITE_PATTERN_HELP \
	("<off> <len>\n\n" \
	 "Writes a pattern of (0x00 0x01 .. 0xFF 0x00 ..) of length" \
	 "<len> at the offset <off>.\n" \
	 "Unaligned writing is used, i.e. protection and erasing are automated." \
	 "This command is designed to test writing to large flash pages.")
#define SET_DEV_HELP \
	("<device_name>\n\n" \
	 "Set flash device by name. If a flash device was not found," \
	 " this command must be run first to bind a device to this module.")

#if (CONFIG_SHELL_ARGC_MAX > 4)
#define ARGC_MAX (CONFIG_SHELL_ARGC_MAX - 4)
#else
#error Please increase CONFIG_SHELL_ARGC_MAX parameter.
#endif

static const struct device *flash_device;

#define RW_TEST_PATTERN_SIZE 4092
static uint8_t test_array[RW_TEST_PATTERN_SIZE];
static bool test_repeat = true;

static int check_flash_device(const struct shell *shell)
{
	if (flash_device == NULL) {
		PR_ERROR(shell, "Flash device is unknown."
				" Run set_device first.\n");
		return -ENODEV;
	}
	return 0;
}

static void dump_buffer(const struct shell *shell, uint8_t *buf, size_t size)
{
	bool newline = false;
	uint8_t *p = buf;

	while (size >= 16) {
		PR_SHELL(shell, "%02x %02x %02x %02x | %02x %02x %02x %02x |"
		       "%02x %02x %02x %02x | %02x %02x %02x %02x\n",
		       p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7],
			   p[8], p[9], p[10], p[11], p[12], p[13], p[14], p[15]);
		p += 16;
		size -= 16;
	}
	if (size >= 8) {
		PR_SHELL(shell, "%02x %02x %02x %02x | %02x %02x %02x %02x\n",
		       p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7]);
		p += 8;
		size -= 8;
		newline = true;
	}
	if (size > 4) {
		PR_SHELL(shell, "%02x %02x %02x %02x | ",
		       p[0], p[1], p[2], p[3]);
		p += 4;
		size -= 4;
		newline = true;
	}
	while (size--) {
		PR_SHELL(shell, "%02x ", *p++);
		newline = true;
	}
	if (newline) {
		PR_SHELL(shell, "\n");
	}
}

static int parse_ul(const char *str, unsigned long *result)
{
	char *end;
	unsigned long val;

	val = strtoul(str, &end, 0);

	if (*str == '\0' || *end != '\0') {
		return -EINVAL;
	}

	*result = val;
	return 0;
}

static int parse_u8(const char *str, uint8_t *result)
{
	unsigned long val;

	if (parse_ul(str, &val) || val > 0xff) {
		return -EINVAL;
	}
	*result = (uint8_t)val;
	return 0;
}

/* Read bytes, dumping contents to console and printing on error. */
static int do_read(const struct shell *shell, off_t offset, size_t len)
{
	uint8_t buf[64];
	int ret;

	while (len > sizeof(buf)) {
		ret = flash_read(flash_device, offset, buf, sizeof(buf));
		if (ret) {
			goto err_read;
		}
		dump_buffer(shell, buf, sizeof(buf));
		len -= sizeof(buf);
		offset += sizeof(buf);
	}
	ret = flash_read(flash_device, offset, buf, len);
	if (ret) {
		goto err_read;
	}
	dump_buffer(shell, buf, len);
	return 0;

 err_read:
	PR_ERROR(shell, "flash_read error: %d\n", ret);
	return ret;
}

/* Erase area and printing on error. */
static int do_erase(const struct shell *shell, off_t offset, size_t size)
{
	int ret;

	ret = flash_erase(flash_device, offset, size);
	if (ret) {
		PR_ERROR(shell, "flash_erase failed (err:%d).\n", ret);
		return ret;
	}

	return ret;
}

/* Write bytes and printing on error. */
static int do_write(const struct shell *shell, off_t offset, uint8_t *buf,
		    size_t len, bool read_back)
{
	int ret;

	ret = flash_write(flash_device, offset, buf, len);
	if (ret) {
		PR_ERROR(shell, "flash_write failed (err:%d).\n", ret);
		return ret;
	}

	if (read_back) {
		PR_SHELL(shell, "Reading back written bytes:\n");
		ret = do_read(shell, offset, len);
	}
	return ret;
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

	op_buf = (uint8_t *)k_malloc(sector_sz);
	if (op_buf == NULL) {
		printk("heap full %d %d\n", __LINE__, sector_sz);
		ret = -EINVAL;
		goto end;
	}

	read_back_buf = (uint8_t *)k_malloc(sector_sz);
	if (read_back_buf == NULL) {
		printk("heap full %d %d\n", __LINE__, sector_sz);
		ret = -EINVAL;
		goto end;
	}

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

	if (op_buf != NULL)
		k_free(op_buf);
	if (read_back_buf != NULL)
		k_free(read_back_buf);

	return ret;
}

static int cmd_get_sector_size(const struct shell *shell, size_t argc,
				char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	int err = check_flash_device(shell);

	if (!err) {
		PR_SHELL(shell, "%d\n",
			 flash_get_write_block_size(flash_device));
	}

	return err;
}

static int cmd_read(const struct shell *shell, size_t argc, char **argv)
{
	int err = check_flash_device(shell);
	unsigned long offset, len;

	if (err) {
		goto exit;
	}

	if (parse_ul(argv[1], &offset) || parse_ul(argv[2], &len)) {
		PR_ERROR(shell, "Invalid arguments.\n");
		err = -EINVAL;
		goto exit;
	}

	err = do_read(shell, offset, len);

exit:
	return err;
}

static int cmd_erase(const struct shell *shell, size_t argc, char **argv)
{
	int err = check_flash_device(shell);
	unsigned long offset;
	unsigned long size;

	if (err) {
		goto exit;
	}

	if (parse_ul(argv[1], &offset) || parse_ul(argv[2], &size)) {
		PR_ERROR(shell, "Invalid arguments.\n");
		err = -EINVAL;
		goto exit;
	}

	err = do_erase(shell, (off_t)offset, (size_t)size);
exit:
	return err;
}

static int cmd_write_template(const struct shell *shell, size_t argc, char **argv, bool unaligned)
{
	unsigned long i, offset;
	uint8_t buf[ARGC_MAX];

	int err = check_flash_device(shell);

	if (err) {
		goto exit;
	}

	err = parse_ul(argv[1], &offset);
	if (err) {
		PR_ERROR(shell, "Invalid argument.\n");
		goto exit;
	}

	if ((argc - 2) > ARGC_MAX) {
		/* Can only happen if Zephyr limit is increased. */
		PR_ERROR(shell, "At most %lu bytes can be written.\n"
				"In order to write more bytes please increase"
				" parameter: CONFIG_SHELL_ARGC_MAX.\n",
			 (unsigned long)ARGC_MAX);
		err = -EINVAL;
		goto exit;
	}

	/* skip cmd name and offset */
	argc -= 2;
	argv += 2;
	for (i = 0; i < argc; i++) {
		if (parse_u8(argv[i], &buf[i])) {
			PR_ERROR(shell, "Argument %lu (%s) is not a byte.\n"
					"Bytes shall be passed in decimal"
					" notation.\n",
				 i + 1, argv[i]);
			err = -EINVAL;
			goto exit;
		}
	}

	if (!unaligned) {
		err = do_write(shell, offset, buf, i, true);
	} else {
		err = do_update(flash_device, offset, buf, i);
	}

exit:
	return err;
}

static int cmd_write(const struct shell *shell, size_t argc, char **argv)
{
	return cmd_write_template(shell, argc, argv, false);
}

static int cmd_rw_pattern_test(const struct shell *shell, size_t argc, char **argv)
{
	int ret = 0;
	uint32_t i;

	if (test_repeat) {
		for (i = 0; i < RW_TEST_PATTERN_SIZE; i++) {
			test_array[i] = 'a' + (i % 26);
		}
	} else {
		for (i = 0; i < RW_TEST_PATTERN_SIZE; i++) {
			test_array[i] = 'z' - (i % 26);
		}
	}

	ret = do_update(flash_device, 0xFE100, test_array, RW_TEST_PATTERN_SIZE);
	if (ret != 0)
		PR_ERROR(shell, "RW test fail\n");
	else
		PR_SHELL(shell, "RW test pass\n");

	test_repeat ^= true;

	return ret;
}

static int cmd_write_pattern(const struct shell *shell, size_t argc, char **argv)
{
	int err = check_flash_device(shell);
	unsigned long offset, len, i;
	static uint8_t *buf;

	if (err) {
		goto exit;
	}

	if (parse_ul(argv[1], &offset) || parse_ul(argv[2], &len)) {
		PR_ERROR(shell, "Invalid arguments.\n");
		err = -EINVAL;
		goto exit;
	}

	buf = k_malloc(len);

	if (!buf) {
		PR_ERROR(shell, "No heap memory for data pattern\n");
		err = -ENOMEM;
		goto exit;
	}

	for (i = 0; i < len; i++) {
		buf[i] = i & 0xFF;
	}

	err = do_update(flash_device, offset, buf, i);

	k_free(buf);

exit:
	return err;
}

static int cmd_set_dev(const struct shell *shell, size_t argc, char **argv)
{
	const struct device *dev;
	const char *name;

	name = argv[1];

	/* Run command. */
	dev = device_get_binding(name);
	if (!dev) {
		PR_ERROR(shell, "No device named %s.\n", name);
		return -ENOEXEC;
	}
	if (flash_device != dev) {
		PR_SHELL(shell, "Leaving behind device %s and changing to %s\n",
			 flash_device->name, dev->name);
	}
	flash_device = dev;

	return 0;
}

void main(void)
{
	flash_device =
		device_get_binding(DT_CHOSEN_ZEPHYR_FLASH_CONTROLLER_LABEL);
	if (flash_device) {
		printk("Found flash controller %s.\n",
			DT_CHOSEN_ZEPHYR_FLASH_CONTROLLER_LABEL);
		printk("Flash I/O commands can be run.\n");
	} else {
		printk("**No flash controller found!**\n");
		printk("Run set_device <name> to specify one "
		       "before using other commands.\n");
	}
}


SHELL_STATIC_SUBCMD_SET_CREATE(sub_sf,
	/* Alphabetically sorted to ensure correct Tab autocompletion. */
	SHELL_CMD_ARG(erase, NULL, ERASE_HELP, cmd_erase, 3, 0),
	SHELL_CMD_ARG(read, NULL, READ_HELP, cmd_read, 3, 0),
	SHELL_CMD_ARG(set_device, NULL, SET_DEV_HELP, cmd_set_dev, 2, 0),
	SHELL_CMD_ARG(write, NULL, WRITE_HELP, cmd_write, 3, 255),
	SHELL_CMD_ARG(erase_size, NULL, SECTOR_SIZE_HELP,
							cmd_get_sector_size, 1, 0),
	SHELL_CMD_ARG(rw_test, NULL, WRITE_UNALIGNED_HELP,
							cmd_rw_pattern_test, 0, 0),
	SHELL_CMD_ARG(write_pattern, NULL, WRITE_PATTERN_HELP,
							cmd_write_pattern, 3, 255),
	SHELL_SUBCMD_SET_END /* Array terminated. */
);

SHELL_CMD_REGISTER(aspeed_sf, &sub_sf, "ASPEED SPI Flash related commands.", NULL);
