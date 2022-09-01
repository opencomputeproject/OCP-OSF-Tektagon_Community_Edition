/*
 * Copyright (c) 2021 Aspeed Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <tc_util.h>
#include "ast_test.h"
#include <drivers/flash.h>
#include <kernel.h>
#include <sys/util.h>

#include <stdlib.h>
#include <string.h>
#include <zephyr.h>

#define LOG_MODULE_NAME spi_flash_test

#include <logging/log.h>
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

#define TEST_ARR_SIZE 0x1000
#define UPDATE_TEST_PATTERN_SIZE (TEST_ARR_SIZE - 4)

static uint8_t __aligned(4) test_arr[TEST_ARR_SIZE];

static char *flash_device[6] = {
	"fmc_cs0",
	"fmc_cs1",
	"spi1_cs0",
	"spi1_cs1",
	"spi2_cs0",
	"spi2_cs1"
};

static int do_erase_write_verify(const struct device *flash_device,
			uint32_t op_addr, uint8_t *write_buf, uint8_t *read_back_buf,
			uint32_t erase_sz)
{
	uint32_t ret = 0;
	uint32_t i;

	ret = flash_erase(flash_device, op_addr, erase_sz);
	if (ret != 0) {
		LOG_ERR("[%s][%d] erase failed at %d.",
			__func__, __LINE__, op_addr);
		goto end;
	}

	ret = flash_write(flash_device, op_addr, write_buf, erase_sz);
	if (ret != 0) {
		LOG_ERR("[%s][%d] write failed at %d.",
			__func__, __LINE__, op_addr);
		goto end;
	}

	flash_read(flash_device, op_addr, read_back_buf, erase_sz);
	if (ret != 0) {
		LOG_ERR("[%s][%d] write failed at %d.",
			__func__, __LINE__, op_addr);
		goto end;
	}

	if (memcmp(write_buf, read_back_buf, erase_sz) != 0) {
		ret = -EINVAL;
		LOG_ERR("ERROR: %s %d fail to write flash at 0x%x",
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

	LOG_INF("Udpating %s...", flash_device->name);

	if (flash_sz < flash_offset + len) {
		LOG_ERR("ERROR: update boundary exceeds flash size. (%d, %d, %d)",
			flash_sz, flash_offset, len);
		ret = -EINVAL;
		goto end;
	}

	op_buf = (uint8_t *)malloc(sector_sz);
	if (op_buf == NULL) {
		LOG_ERR("heap full %d %d", __LINE__, sector_sz);
		ret = -EINVAL;
		goto end;
	}

	read_back_buf = (uint8_t *)malloc(sector_sz);
	if (read_back_buf == NULL) {
		LOG_ERR("heap full %d %d", __LINE__, sector_sz);
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
	LOG_INF("Update %s.", ret ? "FAILED" : "done");

	if (op_buf != NULL)
		free(op_buf);
	if (read_back_buf != NULL)
		free(read_back_buf);

	return ret;
}

int test_spi(int count, enum aspeed_test_type type)
{
	int ret = 0;
	const struct device *flash_dev;
	static bool test_repeat = true;
	uint32_t i;
	uint32_t exec_cnt = 0;

	LOG_INF("%s, count: %d, type: %d", __func__, count, type);

	if (type != AST_TEST_CI)
		return AST_TEST_PASS;

	if (type == AST_TEST_CI) {
		count = 10;
		while (count > 0) {
			LOG_INF("flash update count = %d", exec_cnt);
			if (test_repeat) {
				for (i = 0; i < UPDATE_TEST_PATTERN_SIZE; i++) {
					test_arr[i] = 'a' + (i % 26);
				}
			} else {
				for (i = 0; i < UPDATE_TEST_PATTERN_SIZE; i++) {
					test_arr[i] = 'z' - (i % 26);
				}
			}

			for (i = 0; i < 6; i++) {
				flash_dev = device_get_binding(flash_device[i]);
				if (!flash_dev) {
					LOG_ERR("No device named %s.", flash_device[i]);
					return -ENOEXEC;
				}
				ret = do_update(flash_dev, 0xE0000,
						test_arr, UPDATE_TEST_PATTERN_SIZE);
				if (ret != 0) {
					LOG_ERR("RW test fail");
					goto end;
				}
			}

			LOG_INF("RW test pass");

			test_repeat ^= true;
			count--;
			exec_cnt++;
		}
	}

end:
	return ret;
}
