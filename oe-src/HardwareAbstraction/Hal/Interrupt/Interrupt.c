// ***********************************************************************
// *                                                                     *
// *                  Copyright (c) 1985-2022, AMI.                      *
// *                                                                     *
// *      All rights reserved. Subject to AMI licensing agreement.       *
// *                                                                     *
// ***********************************************************************
/**@file
 * This file contains the Interrupt Handling functions
 */

#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include "Interrupt.h"

/**
 *	Function to Timer Interrupt
 */
static int GetTimerInterrupt(void)
{
	return GetTimerInterruptHook();
}

/**
 *  Initialize Interrupt Interface
 */
int InterruptInterface(struct InterruptInterface *Interrupt)
{
	if (Interrupt == NULL) {
		return INTERRUPT_INVALID_ARGUMENT;
	}

	// memset (Interrupt, 0, sizeof (struct Interrupt));

	Interrupt->TimerInterrupt = GetTimerInterrupt;

	return 0;
}
