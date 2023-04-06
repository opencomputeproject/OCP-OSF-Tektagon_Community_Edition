//***********************************************************************
//*                                                                     *
//*                  Copyright (c) 1985-2022, AMI.                      *
//*                                                                     *
//*      All rights reserved. Subject to AMI licensing agreement.       *
//*                                                                     *
//***********************************************************************
/**@file
 * This file contains the Logging Handling functions
 */
#include <stdio.h>
#include <logging/logging.h>
#include <logging/debug_log.h>
#include <logging/logging_flash.h>
#include "CommonFlash/CommonFlash.h"
#include "CommonLogging.h"

#ifdef CERBERUS_PREVIOUS_VERSION
#include "Logging/LoggingWrapper.h"

#define LOGGING_FLASH_DEVICE_SIZE       TE_LOGGING_FLASH_SIZE   // logging flash device size supported
#define LOGGING_FLASH_BASE_ADDRESS      TE_LOGGING_BASE_ADD     // flash base address for logging
#define ROT_INTERNAL_LOG        TE_ROT_INTERNAL_LOG             // flash base address for logging
#endif

static struct spi_flash LogFlash;	// logging flash implementations info

#ifndef CERBERUS_PREVIOUS_VERSION
static struct logging_flash_state LogFlashstate;
#endif

struct flash_master Spi;			// interface to the SPI master connected to a flash device.

/**
 * @brief Initializes logging features.
 * 
 * Initializes flash device and APIs.
 * input argument of logging will be updated as logging flash if flash device and logging flash are success to initialize.
 *
 * @return Completion status, 0 if success or an error code.
 */
int LogingFlashInit (struct logging_flash *Logging)
{
	int status = 0;	// initialization status
	
	printf("\nlogging_flash_wrapper_init\n ");

	if( Logging == NULL )
	{
		return -1;	// empty logging argument
	}

	status = FlashMasterInit(&Spi);
	if( status != 0 )
	{
		return status;
	}

	/* Initializes SPI flash access functions and corresponding commands.
	 * spi variable is unused argument in flash_wrapper_init function. */
	status = FlashInit(&LogFlash, &Spi);

	if( status != 0 )
	{
		return status;	// failed to flash wrapper initialization
	}

#ifndef CERBERUS_PREVIOUS_VERSION
	LogFlash.state->device_id[0] = ROT_INTERNAL_LOG;// Access fmc_cs0 ROT_INTERNAL_LOG// move to wrapper
#else
	LogFlash.device_id[0] = ROT_INTERNAL_LOG;// Access fmc_cs0 ROT_INTERNAL_LOG// move to wrapper
#endif

	/* retrieves flash device size. */
#if 0
	status = logFlash.spi.base.get_device_size((struct flash *)&LogFlash, &LogFlash.device_size);
#else
	status = spi_flash_set_device_size (&LogFlash, LOGGING_FLASH_DEVICE_SIZE);	// set flash device size
#endif

	if( status != 0 )
	{
		return status;	// failed to retrieves flash device size
	}
#ifndef CERBERUS_PREVIOUS_VERSION
	status = logging_flash_init(Logging, &LogFlash, &LogFlashstate, LOGGING_FLASH_BASE_ADDRESS);
#else
	status = logging_flash_init(Logging, &LogFlash, LOGGING_FLASH_BASE_ADDRESS);
#endif
	if( status != 0 )
	{
		return status;	// failed to initializes logging flash
	}
	return 0;	// debug_log info updated
}

int DebugInit(void)
{
	debug_log = malloc(sizeof(struct logging_flash));

	if( debug_log == NULL )
		return __LINE__;

	if( LogingFlashInit( debug_log ) )
		return __LINE__;

	if ( debug_log_clear() )
		return __LINE__;
}
