//***********************************************************************
//*                                                                     *
//*                  Copyright (c) 1985-2022, AMI.                      *
//*                                                                     *
//*      All rights reserved. Subject to AMI licensing agreement.       *
//*                                                                     *
//***********************************************************************
/**@file
 * This file contains the Flash Master Handling functions
 */
#include "flash/flash_master.h"
#include <Flash/FlashWrapper.h>
#include <CommonFlash/CommonFlash.h>
#ifndef CERBERUS_PREVIOUS_VERSION 
#include <flash/flash_aspeed.h>
#endif
/**
 * Get a set of capabilities supported by the SPI master.
 *
 * @param spi The SPI master to query.
 *
 * @return A capabilities bitmask for the SPI master.
 */
uint32_t FlashMasterCapabilities (struct flash_master *spi)
{
	return Wrapper_flash_master_capabilities(spi);
}

/**
 * Get a set of capabilities supported by the SPI master.
 *
 * @param spi The SPI master to query.
 *
 * @return A capabilities bitmask for the SPI master.
 */
uint32_t SPICommandXfer (struct spi_flash *flash, const struct flash_xfer *Xfer)
{
	return Wrapper_SPI_Command_Xfer(flash, Xfer);
}

int FlashMasterInit(struct FlashMaster *spi)
{
	spi->base.capabilities = FlashMasterCapabilities;
#ifndef CERBERUS_PREVIOUS_VERSION 
	spi->base.xfer = SPI_Command_Xfer;
#endif

	return 0;
}
