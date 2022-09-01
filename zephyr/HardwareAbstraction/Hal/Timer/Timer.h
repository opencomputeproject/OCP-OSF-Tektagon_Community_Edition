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

#ifndef TIMER_H_
#define TIMER_H_

#include <stdlib.h>
#include <stddef.h>
#include <string.h>

/**
 * Timer Interface
 */
struct TimerInterface {
	/**
	 * Function to Start Basic Timer.
	 *
	 * @param Timer Interface 
	 *
	 * @return 0 
	 */
	int (*BasicTimerStart) ();

	/**
	 * Function to Stop Basic Timer.
	 *
	 * @param  Timer Interface
	 *
	 * @return 0
	 */
	int (*BasicTimerStop) ();
};

int BasicTimerInit(struct TimerInterface *Timer);

/**
 * Error codes that can be generated by a Timer.
 */
enum {
	TIMER_INVALID_ARGUMENT = 0x00           /**< Input parameter is null or not valid. */
};
#endif /* TIMER_H_ */
