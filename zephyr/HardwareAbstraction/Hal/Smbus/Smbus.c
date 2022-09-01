// ***********************************************************************
// *                                                                     *
// *                  Copyright (c) 1985-2022, AMI.                      *
// *                                                                     *
// *      All rights reserved. Subject to AMI licensing agreement.       *
// *                                                                     *
// ***********************************************************************
/**@file
 * This file contains the Smbus Handling functions
 */

#include "Smbus.h"

/**
 *	Function to Smbus Read.
 */
static int SmbusReadAccess(void)
{
	return SmbusReadAccessHook();
}

/**
 *	Function to Smbus Write.
 */
static int SmbusWriteAccess(void)
{
	return SmbusWriteAccessHook();
}

/**
 *  Initialize Smbus
 */
int SmbusInit(struct SmbusInterface *Smbus)
{
	if (Smbus == NULL) {
		return SMBUS_INVALID_ARGUMENT;
	}

	// memset (Smbus, 0, sizeof (struct Smbus));

	Smbus->SmbusRead = SmbusReadAccess;
	Smbus->SmbusWrite = SmbusWriteAccess;

	return 0;
}
