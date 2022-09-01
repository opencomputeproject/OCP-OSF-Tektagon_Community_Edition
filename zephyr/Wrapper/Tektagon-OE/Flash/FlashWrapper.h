
//***********************************************************************
//*                                                                     *
//*                  Copyright (c) 1985-2022, AMI.                      *
//*                                                                     *
//*      All rights reserved. Subject to AMI licensing agreement.       *
//*                                                                     *
//***********************************************************************
/**@file
 * This file contains the Flash Wrapper Handling functions
 */

#ifndef FLASH_WRAPPER_COMMON_H_
#define FLASH_WRAPPER_COMMON_H_

#include "flash/spi_flash.h"

int Wrapper_spi_flash_get_device_size (struct spi_flash *flash, uint32_t *bytes);
int Wrapper_spi_flash_read (struct spi_flash *flash, uint32_t address, uint8_t *data, size_t length);
int Wrapper_spi_flash_get_page_size (struct spi_flash *flash, uint32_t *bytes);
int Wrapper_spi_flash_minimum_write_per_page (struct spi_flash *flash, uint32_t *bytes);
int Wrapper_spi_flash_write (struct spi_flash *flash, uint32_t address, const uint8_t *data, size_t length);
int Wrapper_spi_flash_get_sector_size (struct spi_flash *flash, uint32_t *bytes);
int Wrapper_spi_flash_sector_erase (struct spi_flash *flash, uint32_t sector_addr);
int Wrapper_spi_flash_get_block_size (struct spi_flash *flash, uint32_t *bytes);
int Wrapper_spi_flash_block_erase (struct spi_flash *flash, uint32_t block_addr);
int Wrapper_spi_flash_chip_erase (struct spi_flash *flash);
uint32_t Wrapper_flash_master_capabilities (struct flash_master *spi);


#endif /* FLASH_WRAPPER_COMMON_H_ */
