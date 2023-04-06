//***********************************************************************
//*                                                                     *
//*                  Copyright (c) 1985-2022, AMI.                      *
//*                                                                     *
//*      All rights reserved. Subject to AMI licensing agreement.       *
//*                                                                     *
//***********************************************************************
/**@file
 * This file contains the Flash Handling functions
 */

#include "CommonFlash.h"

#include <stdlib.h>
#include <string.h>

#include "Flash/FlashWrapper.h"
#include "flash/flash_common.h"
#include "flash/flash_logging.h"
#include "flash/flash_master.h"
#include "flash/spi_flash.h"

int SpiCommandRead(struct spi_flash *flash)
{
	return WrapperSpiCommandRead();
}

int SpiCommandWrite(struct spi_flash *flash)
{
	return WrapperSpiCommandWrite();
}

int Spi4KEraseSector(struct spi_flash *flash)
{
	return WrapperSpi4KEraseSector();
}

int Spi4KEraseBlock(struct spi_flash *flash)
{
	return WrapperSpi4KEraseBlock();
}

int SpiEnterPowerDown(struct spi_flash *flash)
{
	return WrapperSpiEnterPowerDown();
}

int SpiReleasePowerDown(struct spi_flash *flash)
{
	return WrapperSpiReleasePowerDown();
}

int SpiCapabilities(struct spi_flash *flash)
{
	return WrapperSpiCapabilities();
}

int SpiGetDeviceId(struct spi_flash *flash)
{
	return WrapperSpiGetDeviceId();
}
/**
 * Get the size of the flash device.
 *
 * @param flash The flash to query.
 * @param bytes The buffer that will hold the number of bytes in the device.
 *
 * @return 0 if the device size was successfully read or an error code.
 */
int SpiFlashGetDeviceSize (struct spi_flash *flash, uint32_t *bytes)
{

	return Wrapper_spi_flash_get_device_size(flash,bytes);
}

/**
 * Read data from the SPI flash.
 *
 * @param flash The flash to read from.
 * @param address The address to start reading from.
 * @param data The buffer to hold the data that has been read.
 * @param length The number of bytes to read.
 *
 * @return 0 if the bytes were read from flash or an error code.
 */
int SpiFlashRead (struct spi_flash *flash, uint32_t address, uint8_t *data, size_t length)
{

	return Wrapper_spi_flash_read(flash,address,data,length);
}

/**
 * Get the size of a flash page for write operations.
 *
 * @param flash The flash to query.
 * @param bytes Output for the number of bytes in a flash page.
 *
 * @return 0 if the page size was successfully read or an error code.
 */
int SpiFlashGetPageSize (struct spi_flash *flash, uint32_t *bytes)
{
	return Wrapper_spi_flash_get_page_size(flash,bytes);
}

/**
 * Get the minimum number of bytes that must be written to a single flash page.  Writing fewer bytes
 * than the minimum to any page will still result in a minimum sized write to flash. The extra bytes
 * that were written must be erased before they can be written again.
 *
 * @param flash The flash to query.
 * @param bytes Output for the minimum number of bytes for a page write.
 *
 * @return 0 if the minimum write size was successfully read or an error code.
 */
int SpiFlashMinimumWritePerPage (struct spi_flash *flash, uint32_t *bytes)
{
	return Wrapper_spi_flash_minimum_write_per_page(flash,bytes);
}

/**
 * Write data to the SPI flash.  The flash needs to be erased prior to writing.
 *
 * @param flash The flash to write to.
 * @param address The address to start writing to.
 * @param data The data to write.
 * @param length The number of bytes to write.
 *
 * @return The number of bytes written to the flash or an error code.  Use ROT_IS_ERROR to check the
 * return value.
 */
int SpiFlashWrite (struct spi_flash *flash, uint32_t address, const uint8_t *data, size_t length)
{
	return Wrapper_spi_flash_write(flash,address,data,length);
}

/**
 * Get the size of a flash sector for erase operations.
 *
 * @param flash The flash to query.
 * @param bytes Output for the number of bytes in a flash sector.
 *
 * @return 0 if the sector size was successfully read or an error code.
 */
int SpiFlashGetSectorSize (struct spi_flash *flash, uint32_t *bytes)
{
	return Wrapper_spi_flash_get_sector_size(flash,bytes);
}

/**
 * Erase a 4kB sector of flash.
 *
 * @param flash The flash to erase.
 * @param sector_addr An address within the sector to erase.
 *
 * @return 0 if the sector was erased or an error code.
 */
int SpiFlashSectorErase (struct spi_flash *flash, uint32_t sector_addr)
{

	return Wrapper_spi_flash_sector_erase(flash, sector_addr);
}

/**
 * Get the size of a flash block for erase operations.
 *
 * @param flash The flash to query.
 * @param bytes Output for the number of bytes in a flash block.
 *
 * @return 0 if the block size was successfully read or an error code.
 */
int SpiFlashGetBlockSize (struct spi_flash *flash, uint32_t *bytes)
{
	return Wrapper_spi_flash_get_block_size(flash, bytes);
}

/**
 * Erase a 64kB block of flash.
 *
 * @param flash The flash to erase.
 * @param block_addr An address within the block to erase.
 *
 * @return 0 if the block was erased or an error code.
 */
int SpiFlashBlockErase (struct spi_flash *flash, uint32_t block_addr)
{
	return Wrapper_spi_flash_block_erase(flash,block_addr);
}

/**
 * Erase the entire flash chip.
 *
 * @param flash The flash to erase.
 *
 * @return 0 if the flash chip was erased or an error code.
 */
int SpiFlashChipErase (struct spi_flash *flash)
{
	return Wrapper_spi_flash_chip_erase(flash);
}

/**
 * Erase the Flash Init.
 *
 * @param IN struct SpiEngine *Flash
 * @param IN struct FlashMaster *Spi
 *
 * @return 0 if the flash chip was erased or an error code.
 */
int FlashInit (struct SpiEngine *Flash, struct FlashMaster *Spi)
{
	int status;
	uint32_t bytes;

	if ((Flash == NULL) || (Spi == NULL)) {
		return SPI_FLASH_INVALID_ARGUMENT;
	}

	memset (Flash, 0, sizeof (struct SpiEngine));

#ifndef CERBERUS_PREVIOUS_VERSION
	status = platform_mutex_init (&Flash->spi.state->lock);
	if (status != 0)
		return status;

	Flash->spi.state->command.read = SpiCommandRead;
	Flash->spi.state->command.write = SpiCommandWrite;
	Flash->spi.state->command.erase_sector = Spi4KEraseSector;
	Flash->spi.state->command.erase_block = Spi4KEraseBlock;
	Flash->spi.state->command.enter_pwrdown = SpiEnterPowerDown;
	Flash->spi.state->command.release_pwrdown = SpiReleasePowerDown;
	//Flash->spi.spi->xfer =  Spi->base.xfer;
	Flash->spi.state->capabilities = Spi->base.capabilities;
	Flash->spi.state->device_id[0] = SpiGetDeviceId;
	Flash->spi.state->capabilities = SpiCapabilities;
#else
  status = platform_mutex_init (&Flash->spi.lock);
	if (status != 0)
		return status;

	Flash->spi.command.read = SpiCommandRead;
	Flash->spi.command.write = SpiCommandWrite;
	Flash->spi.command.erase_sector = Spi4KEraseSector;
	Flash->spi.command.erase_block = Spi4KEraseBlock;
	Flash->spi.command.enter_pwrdown = SpiEnterPowerDown;
	Flash->spi.command.release_pwrdown = SpiReleasePowerDown;
	
	Flash->spi.spi->xfer =  Spi->base.xfer;
	Flash->spi.spi->capabilities = Spi->base.capabilities;
	Flash->spi.device_id[0] = SpiGetDeviceId;
	Flash->spi.capabilities = SpiCapabilities;
#endif
	Flash->spi.base.get_device_size = SpiFlashGetDeviceSize;
	Flash->spi.base.read = SpiFlashRead;
	Flash->spi.base.get_page_size = SpiFlashGetPageSize;
	Flash->spi.base.minimum_write_per_page = SpiFlashMinimumWritePerPage;
	Flash->spi.base.write = SpiFlashWrite;
	Flash->spi.base.get_sector_size = SpiFlashGetSectorSize;
	Flash->spi.base.sector_erase = SpiFlashSectorErase;
	Flash->spi.base.get_block_size = SpiFlashGetBlockSize;
	Flash->spi.base.block_erase = SpiFlashBlockErase;
	Flash->spi.base.chip_erase = SpiFlashChipErase;

	return 0;
}
