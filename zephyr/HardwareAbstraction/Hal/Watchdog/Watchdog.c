// ***********************************************************************
// *                                                                     *
// *                  Copyright (c) 1985-2022, AMI.                      *
// *                                                                     *
// *      All rights reserved. Subject to AMI licensing agreement.       *
// *                                                                     *
// ***********************************************************************
/**@file
 * This file contains the Watchdog Handling functions
 */

#include "Watchdog.h"

/**
 *	Function to Watchdog Read.
 */
static int WatchdogFeed(void)
{
	return WatchdogFeedHook();
}

/**
 *	Function to Watchdog Disable
 */
static int WatchdogDisable(void)
{
	return WatchdogDisable();
}

/**
 *  Initialize Watchdog
 */
int WatchdogInit(struct WatchdogInterface *Watchdog)
{
	if (Watchdog == NULL) {
		return WATCHDOG_INVALID_ARGUMENT;
	}

	// memset (Watchdog, 0, sizeof (struct Watchdog));

	Watchdog->WatchdogFeed = WatchdogFeed;
	Watchdog->WatchdogDisable = WatchdogDisable;

	return 0;
}
