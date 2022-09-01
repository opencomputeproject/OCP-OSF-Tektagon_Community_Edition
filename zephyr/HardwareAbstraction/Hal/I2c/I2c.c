//***********************************************************************
//*                                                                     *
//*                  Copyright (c) 1985-2022, AMI.                      *
//*                                                                     *
//*      All rights reserved. Subject to AMI licensing agreement.       *
//*                                                                     *
//***********************************************************************
/**@file
 * This file contains the I2c Handling functions
 */

#include "I2c.h"
#include "i2c/i2c_master_interface.h"
#include "I2c/I2cWrapper.h"

/* I2cSlaveInitSlaveDev
 * I2c salve initial to asp1060 api.
 *
 * @param i2c 			structure i2c_slave_interface
 * @param DevName 		I2C device name, support I2C_0 and I2C_1
 * @param slave_addr	I2c slave device slave address
 *
 * @return 0 if the I2C slave was successfully initialize or an error code.
 */
int I2cSlaveInitSlaveDev(struct i2c_slave_interface *i2c, char *DevName, uint8_t slave_addr)
{
	return I2cSlaveWrapperInitSlaveDev(i2c, DevName, slave_addr);
}
/**
 * Initialize an aspeed I2C slave engine.
 *
 * @param engine The I2C slave to initialize.
 *
 * @return 0 if the I2C slave was successfully initialize or an error code.
 */
int I2cSlaveInit (struct i2c_slave_interface *I2cSlaveEngine)
{
	int status;

	if (I2cSlaveEngine == NULL) {
		return I2C_SLAVE_INVALID_ARGUMENT;
	}

	memset (I2cSlaveEngine, 0, sizeof (I2cSlaveEngine));

	I2cSlaveEngine->InitSlaveDev = I2cSlaveInitSlaveDev;

	return 0;
}

