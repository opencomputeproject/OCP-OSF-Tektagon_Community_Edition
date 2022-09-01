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

//#include <kernel.h>
//#include <sys/util.h>
#include <stdlib.h>
#include <string.h>
#include <zephyr.h>
#include <device.h>
//#include <I2cSlave.h>
#include "I2cWrapper.h"
#include <i2c/I2C_Slave_aspeed.h>
#include <I2c/I2c.h>

/* I2cSlaveWrapperInitSlaveDev
 * I2c salve initial wrapper to asp1060 api.
 *
 * @param i2c 			structure i2c_slave_interface
 * @param DevName 		I2C device name, support I2C_0 and I2C_1
 * @param slave_addr	I2c slave device slave address
 *
 * @return 0 if the I2C slave was successfully initialize or an error code.
 */
int I2cSlaveWrapperInitSlaveDev(struct i2c_slave_interface *i2c, char *DevName, uint8_t slave_addr)
{
	const struct device *dev;

	dev = device_get_binding(DevName);

	if (!dev) {
		return I2C_SLAVE_NO_DEVICE;
	}
	printk("\r\n I2CSlave_wrapper_InitSlaveDev get device\r\n");
    return ast_i2c_slave_dev_init(dev , slave_addr);
}
