// ***********************************************************************
// *                                                                     *
// *                  Copyright (c) 1985-2022, AMI.                      *
// *                                                                     *
// *      All rights reserved. Subject to AMI licensing agreement.       *
// *                                                                     *
// ***********************************************************************
/**@file
 * This file contains the I2c Handling functions
 */

#include "I2c.h"
#include "i2c/i2c_master_interface.h"

/**
 * Execute an I2C write register using provided interface
 *
 * @param i2c I2C master interface to use
 * @param slave_addr I2C slave device address
 * @param reg_addr I2C register address
 * @param reg_addr_len I2C register address length
 * @param data Transaction data buffer
 * @param len Transaction data length
 *
 * @return Transfer status, 0 if success or an error code.
 */
int I2cMasterWriteRegister(struct i2c_master_interface *i2c, uint16_t SlaveAddress, uint32_t Register,
			   size_t RegisterAddressLength, uint8_t *Data, size_t Length)
{
	return I2cEngineMasterWriteRegister(SlaveAddress, Register, RegisterAddressLength, Data, Length);
}

/**
 * Execute an I2C read register using provided interface
 *
 * @param i2c I2C master interface to use
 * @param slave_addr I2C slave device address
 * @param reg_addr I2C register address
 * @param reg_addr_len I2C register address length
 * @param data Transaction data buffer
 * @param len Transaction data length
 *
 * @return Transfer status, 0 if success or an error code.
 */
int I2cMasterReadRegister(struct i2c_master_interface *i2c, uint16_t SlaveAddress, uint32_t Register,
			  uint32_t RegisterAddress, size_t RegisterAddressLength, uint8_t *Data, size_t Length)
{
	return I2cEngineMasterReadRegister(SlaveAddress, Register, RegisterAddressLength, Data, Length);
}

/**
 * Execute an I2C write using provided interface
 *
 * @param i2c I2C master interface to use
 * @param slave_addr I2C slave device address
 * @param data Transaction data buffer
 * @param len Transaction data length
 *
 * @return Transfer status, 0 if success or an error code described in platform_errno.h
 */
int I2cMasterWrite(struct i2c_master_interface *i2c, uint16_t SlaveAddress, uint8_t *Data, size_t Length)
{
	return I2cEngineMasterWrite(SlaveAddress, Data, Length);
}

/**
 * Initialize an aspeed I2C slave engine.
 *
 * @param engine The I2C slave to initialize.
 *
 * @return 0 if the I2C slave was successfully initialize or an error code.
 */
int I2cMasterInit(struct i2c_master_interface *I2cMasterEngine)
{

	if (I2cMasterEngine == NULL) {
		return I2C_MASTER_INVALID_ARGUMENT;
	}

	memset(I2cMasterEngine, 0, sizeof(struct i2c_master_interface));

	I2cMasterEngine->write_reg = I2cMasterWriteRegister;
	I2cMasterEngine->read_reg = I2cMasterReadRegister;
	I2cMasterEngine->write = I2cMasterWrite;

	return 0;
}