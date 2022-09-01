//***********************************************************************
//*                                                                     *
//*                  Copyright (c) 1985-2022, AMI.                      *
//*                                                                     *
//*      All rights reserved. Subject to AMI licensing agreement.       *
//*                                                                     *
//***********************************************************************
/**@file
 * This file contains the Dice Handling functions
 */

#ifndef DICE_H_
#define DICE_H_

#include <stdlib.h>
#include <stdio.h>

#define TRACE printf
/**
 * Dice Interface
 */
struct DiceInterface {
	/**
	 * Function to Dice Status.
	 *
	 * @param  Dice Interface
	 *
	 * @return 0 
	 */
	int (*DiceStatus  ) ();
};

int DiceInit(void);

/**
 * Error codes that can be generated by a DICE.
 */
enum {
	DICE_INVALID_ARGUMENT = 0x00	/**< Input parameter is null or not valid. */
};
#endif /* Dice_H_ */
