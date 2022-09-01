// ***********************************************************************
// *                                                                     *
// *                  Copyright (c) 1985-2022, AMI.                      *
// *                                                                     *
// *      All rights reserved. Subject to AMI licensing agreement.       *
// *                                                                     *
// ***********************************************************************
/**@file
 * This file contains the GPIO Handling functions
 */

#include "Gpio.h"

/**
 *	Function to Set value for the GPIO PIN
 */
static int GpioSetConfiguration(void)
{
	return GpioSetConfigurationHook();
}

/**
 *	 Function to Get value of the GPIO PIN
 */
static int GpioGetConfiguration(void)
{
	return GpioGetConfigurationHook();
}

/**
 *	System Reset
 */
static int SystemReset(void)
{
	return SystemResetHook();
}

/**
 *  Initialize GPIO
 */
int GpioInit(struct GpioInterface *Gpio)
{
	if (Gpio == NULL) {
		return GPIO_INVALID_ARGUMENT;
	}

	// memset (Gpio, 0, sizeof (struct Gpio));

	Gpio->GpioSet = GpioSetConfiguration;
	Gpio->GpioGet = GpioGetConfiguration;
	Gpio->SysReset = SystemReset;

	return 0;
}
