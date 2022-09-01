// ***********************************************************************
// *                                                                     *
// *                  Copyright (c) 1985-2022, AMI.                      *
// *                                                                     *
// *      All rights reserved. Subject to AMI licensing agreement.       *
// *                                                                     *
// ***********************************************************************

#include "SmbusFilter.h"

/**
 *	Function to Enable SmbusFilter.
 */
static int SmbusFilterEnabled(void)
{
	return SmbusFilterEnabledHook();
}

/**
 *	Function to Disable SmbusFilter.
 */
static int SmbusFilterDisabled(void)
{
	return SmbusFilterDisabledHook();
}

/**
 *  Initialize SmbusFilter
 */
int SmbusFilterInit(struct SmbusFilterInterface *SmbusFilter)
{
	if (SmbusFilter == NULL) {
		return SMBUSFILTER_INVALID_ARGUMENT;
	}

	// memset (SmbusFilter, 0, sizeof (struct SmbusFilter));

	SmbusFilter->SmbusFilterEnable = SmbusFilterEnabled;
	SmbusFilter->SmbusFilterDisable = SmbusFilterDisabled;

	return 0;
}
