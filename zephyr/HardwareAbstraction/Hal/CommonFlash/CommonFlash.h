// ***********************************************************************
// *                                                                     *
// *                  Copyright (c) 1985-2022, AMI.                      *
// *                                                                     *
// *      All rights reserved. Subject to AMI licensing agreement.       *
// *                                                                     *
// ***********************************************************************
/**@file
 * This file contains the Flash Handling functions
 */

#ifndef FLASH_COMMON_H_
#define FLASH_COMMON_H_

#include "flash/flash_master.h"
#include "flash/spi_flash.h"

struct FlashMaster {
	struct flash_master base;
};

struct SpiEngine {
	struct spi_flash spi;
};

struct XferEngine {
	struct flash_xfer base;
};

/**
 * Check the requested operation to ensure it is valid for the device.
 */
#define SPI_FLASH_BOUNDS_CHECK(bytes, addr, len)	\
do {												\
	if (addr >= bytes) {							\
		return SPI_FLASH_ADDRESS_OUT_OF_RANGE;		\
	}						 \
							 \
	if ((addr + len) > bytes) {			 \
		return SPI_FLASH_OPERATION_OUT_OF_RANGE; \
	} \
} while (0)
int FlashInit(struct SpiEngine *Spi, struct FlashMaster *Engine);
int FlashMasterInit(struct FlashMaster *spi);

#endif /* FLASH_COMMON_H_ */
