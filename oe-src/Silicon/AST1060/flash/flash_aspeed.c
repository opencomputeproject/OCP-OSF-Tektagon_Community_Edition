/*
 * Copyright (c) 2021 AMI
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <drivers/flash.h>
#include <drivers/spi_nor.h>
#include <flash/flash_aspeed.h>
#include <kernel.h>
#include <sys/util.h>
#include <stdlib.h>
#include <string.h>
#include <zephyr.h>
#include <flash_map.h>
// #include <flash_master.h>
// #include "flash/spi_flash.h"

#define LOG_MODULE_NAME spi_api

#include <logging/log.h>
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

static char *Flash_Devices_List[6] = {
	"spi1_cs0",
	"spi2_cs0",
	"spi2_cs1",
	"spi2_cs2",
	"fmc_cs0",
	"fmc_cs1"
};

static void Data_dump_buf(uint8_t *buf, uint32_t len)
{
	uint32_t i;

	for (i = 0; i < len; i++) {
		printk("%02x ", buf[i]);
		if (i % 16 == 15)
			printk("\n");
	}
	printk("\n");
}

int BMC_PCH_SPI_Command(struct pspi_flash *flash, struct pflash_xfer *xfer)
{
	struct device *flash_device;
	uint8_t DeviceId = flash->device_id[0];
	int AdrOffset = 0, Datalen = 0;
	uint32_t FlashSize = 0;
	int ret = 0;
	char buf[4096];
	uint32_t page_sz = 0;
	uint32_t sector_sz = 0;

	flash_device = device_get_binding(Flash_Devices_List[DeviceId]);
	AdrOffset = xfer->address;
	Datalen = xfer->length;

	switch (xfer->cmd) {
	case SPI_APP_CMD_GET_FLASH_SIZE:
		FlashSize = flash_get_flash_size(flash_device);
		// printk("FlashSize:%x\n",FlashSize);
		return FlashSize;
		break;
#if 0
	case SPI_APP_CMD_GET_FLASH_SECTOR_SIZE:
		page_sz = flash_get_write_block_size(flash_device);
		return page_sz;
		break;
#else
	case MIDLEY_FLASH_CMD_WREN:
		ret = 0;                // bypass as write enabled
		break;
#endif
	case SPI_APP_CMD_GET_FLASH_BLOCK_SIZE:
		page_sz = (flash_get_write_block_size(flash_device) << 4);
		return page_sz;
		break;
	case MIDLEY_FLASH_CMD_READ:
		ret = flash_read(flash_device, AdrOffset, buf, Datalen);
		// Data_dump_buf(buf,Datalen);
		if (xfer->data != NULL) {
			memcpy(xfer->data, buf, Datalen);
		}
		break;
	case MIDLEY_FLASH_CMD_PP:        // Flash Write
		memset(buf, 0xff, Datalen);
		memcpy(buf, xfer->data, Datalen);
		ret = flash_write(flash_device, AdrOffset, buf, Datalen);
		break;
	case MIDLEY_FLASH_CMD_4K_ERASE:
		sector_sz = flash_get_write_block_size(flash_device);
		ret = flash_erase(flash_device, AdrOffset, sector_sz);
		break;
	case MIDLEY_FLASH_CMD_64K_ERASE:
		sector_sz =  (flash_get_write_block_size(flash_device) << 4);
		ret = flash_erase(flash_device, AdrOffset, sector_sz);
		break;
	case MIDLEY_FLASH_CMD_CE:
		FlashSize = flash_get_flash_size(flash_device);
		ret = flash_erase(flash_device, 0, FlashSize);
		break;
	case MIDLEY_FLASH_CMD_RDSR:
		// bypass as flash status are write enabled and not busy
		*xfer->data = 0x02;
		ret = 0;
		break;
	default:
		printk("%d Command is not supported", xfer->cmd);
		break;
	}

	return ret;
}

int FMC_SPI_Command(struct pspi_flash *flash, struct pflash_xfer *xfer)
{
	struct device *flash_device;
	struct flash_area *partition_device;
	uint32_t FlashSize = 0;
	uint32_t page_sz = 0;
	uint32_t sector_sz = 0;
	int AdrOffset = 0;
	int Datalen = 0;
	char buf[4096];
	int err, ret = 0;

	flash_device = device_get_binding(Flash_Devices_List[ROT_SPI]);

	uint8_t DeviceId = flash->device_id[0];

	AdrOffset = xfer->address;
	Datalen = xfer->length;

	if (DeviceId == ROT_INTERNAL_ACTIVE) {
		err = flash_area_open(FLASH_AREA_ID(active), &partition_device);

	} else if (DeviceId == ROT_INTERNAL_RECOVERY) {
		err = flash_area_open(FLASH_AREA_ID(recovery), &partition_device);

	} else if (DeviceId == ROT_INTERNAL_STATE) {
		err = flash_area_open(FLASH_AREA_ID(state), &partition_device);

	} else if (DeviceId == ROT_INTERNAL_INTEL_STATE) {
		err = flash_area_open(FLASH_AREA_ID(intel_state), &partition_device);

	} else if (DeviceId == ROT_INTERNAL_KEY) {
		err = flash_area_open(FLASH_AREA_ID(key), &partition_device);

	} else if (DeviceId == ROT_INTERNAL_LOG) {
		err = flash_area_open(FLASH_AREA_ID(log), &partition_device);

	}

	switch (xfer->cmd) {
	case SPI_APP_CMD_GET_FLASH_SIZE:
		FlashSize = partition_device->fa_size;
		return FlashSize;
		break;

	case MIDLEY_FLASH_CMD_WREN:
		ret = 0;                // bypass as write enabled
		break;

	case MIDLEY_FLASH_CMD_READ:
		ret = flash_area_read(partition_device, AdrOffset, buf, Datalen);
		if (xfer->data != NULL) {
			memcpy(xfer->data, buf, Datalen);
		}
		break;

	case MIDLEY_FLASH_CMD_PP:        // Flash Write
		memset(buf, 0xff, Datalen);
		memcpy(buf, xfer->data, Datalen);
		ret = flash_area_write(partition_device, AdrOffset, buf, Datalen);
		break;

	case MIDLEY_FLASH_CMD_4K_ERASE:
		sector_sz = flash_get_write_block_size(flash_device);
		ret = flash_area_erase(partition_device, AdrOffset, sector_sz);
		break;

	case MIDLEY_FLASH_CMD_64K_ERASE:
		printk("%d Command is not supported", xfer->cmd);
		ret = 0;
		break;

	case MIDLEY_FLASH_CMD_CE:
		printk("%d Command is not supported", xfer->cmd);
		ret = 0;
		break;
	case MIDLEY_FLASH_CMD_RDSR:
		// bypass as flash status are write enabled and not busy
		*xfer->data = 0x02;
		ret = 0;
		break;
	default:
		printk("%d Command is not supported", xfer->cmd);
		break;
	}

	return ret;
}

int SPI_Command_Xfer(struct pspi_flash *flash, struct pflash_xfer *xfer)
{
	struct device *flash_device;
	uint32_t FlashSize = 0;
	uint8_t DeviceId = flash->device_id[0];
	// uint8_t DeviceId=0;
	uint32_t sector_sz = 0;
	uint32_t page_sz = 0;
	int ret = 0, i = 0;
	char buf[4096];
	int AdrOffset = 0, Datalen = 0;
	struct device *dev;

	if (DeviceId == BMC_SPI || DeviceId == PCH_SPI) {
		ret = BMC_PCH_SPI_Command(flash, xfer);

	} else {
		ret = FMC_SPI_Command(flash, xfer);
	}
	return ret;
}

