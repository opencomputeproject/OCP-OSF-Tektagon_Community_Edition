// ***********************************************************************
// *                                                                     *
// *                  Copyright (c) 1985-2022, AMI.                      *
// *                                                                     *
// *      All rights reserved. Subject to AMI licensing agreement.       *
// *                                                                     *
// ***********************************************************************
/**@file
 * This file contains the Spi Handling functions
 */

#include "Spi.h"

/**
 *	Function to read data in SPI port.
 */
static int SpiReadAccess(void)
{
	return SpiReadAccessHook();
}

/**
 *	 Function to write data in SPI port
 */
static int SpiWriteAccess(void)
{
	return SpiWriteAccessHook();
}

/**
 *	Function to erase the particular chip of size 4k.
 */
static int SpiEraseAccess(void)
{
	return SpiEraseAccessHook();
}

/**
 *  Initialize Spi
 */
int SpiInit(struct SpiInterface *Spi)
{
	if (Spi == NULL) {
		return SPI_INVALID_ARGUMENT;
	}

	// memset (Spi, 0, sizeof (struct Spi));
	Spi->SpiRead = SpiReadAccess;
	Spi->SpiWrite = SpiWriteAccess;
	Spi->SpiErase = SpiEraseAccess;

	return 0;
}
