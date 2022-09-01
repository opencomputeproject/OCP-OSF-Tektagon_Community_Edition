//***********************************************************************
//*                                                                     *
//*                  Copyright (c) 1985-2022, AMI.                      *
//*                                                                     *
//*      All rights reserved. Subject to AMI licensing agreement.       *
//*                                                                     *
//***********************************************************************
/**@file
 * This file contains the I2c Wrapper Handling functions
 */

#include "I2cFilterWrapper.h"
#include <stdbool.h>
#include <string.h>

/**
 * @brief Initialize specific device of I2C filter
 *
 * @param i2c_filter_interface I2C filter interface to use
 *
 * @return 0 if the I2C filter failed to initialize or an error code.
 */
static int WrapperI2cFilterInit(struct i2c_filter_interface *filter)
{
	int ret;

	ret = i2c_filter_middleware_init(filter->filter.device_id);

	return ret;
}

/**
 * @brief Enable / disable specific device of I2C filter
 *
 * @param i2c_filter_interface I2C filter interface to use
 * @param en enable I2C filter
 *
 * @return 0 if the I2C filter failed to enable or an error code.
 */
static int WrapperI2cFilterEnable(struct i2c_filter_interface *filter, bool enable)
{
	int ret;

	ret = i2c_filter_middleware_en(filter->filter.device_id, enable);

	return ret;
}

/**
 * @brief Setup whitelist for specific device of I2C filter with provided offset of slave device.
 *                      Index can using on stores every single offset of slave device with each single index.
 *
 * @param i2c_filter_interface I2C filter interface to use
 * @param index Index to stores register address or offset of slave device
 *
 * @return 0 if the I2C filter failed to setup or an error code.
 */
static int WrapperI2cFilterSet(struct i2c_filter_interface *filter, uint8_t index)
{
	int ret;

	ret = i2c_filter_middleware_set_whitelist(filter->filter.device_id, index,
						  filter->filter.slave_addr, filter->filter.whitelist_elements);

	return ret;
}

