// ***********************************************************************
// *                                                                     *
// *                  Copyright (c) 1985-2022, AMI.                      *
// *                                                                     *
// *      All rights reserved. Subject to AMI licensing agreement.       *
// *                                                                     *
// ***********************************************************************
/**@file
 * This file contains the Timer Handling functions
 */

#include "Timer.h"

/**
 *	Function to Start Basic Timer.
 */
static int BasicTimerStart(void)
{
	return BasicTimerStartHook();
}

/**
 *	Function to Start Basic Timer.
 */
static int BasicTimerStop(void)
{
	return BasicTimerStopHook();
}

/**
 *  Initialize Basic Timer
 */
int TimerInit(struct TimerInterface *Timer)
{
	if (Timer == NULL) {
		return TIMER_INVALID_ARGUMENT;
	}

	// memset (Timer, 0, sizeof (struct Timer));

	Timer->BasicTimerStart = BasicTimerStart;
	Timer->BasicTimerStop = BasicTimerStop;

	return 0;
}
