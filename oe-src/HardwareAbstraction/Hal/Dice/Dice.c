// ***********************************************************************
// *                                                                     *
// *                  Copyright (c) 1985-2022, AMI.                      *
// *                                                                     *
// *      All rights reserved. Subject to AMI licensing agreement.       *
// *                                                                     *
// ***********************************************************************
/**@file
 * This file contains the Dice Handling functions
 */

#include "Dice.h"
// #include "DiceWrapper.h"

/**
 *	Function to Dice Status.
 */
int DiceStatus(void)
{
	return DiceStatusHook();
}

/**
 *  Initialize Dice
 */
int DiceInit(struct DiceInterface *Dice)
{
	if (Dice == NULL) {
		return DICE_INVALID_ARGUMENT;
	}

	// memset (Dice, 0, sizeof (struct DiceInterface));

	Dice->DiceStatus = DiceStatus;

	return 0;
}
