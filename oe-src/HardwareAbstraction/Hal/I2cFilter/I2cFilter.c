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

#include "I2cFilter.h"
#include "I2cFilter/I2cFilterWrapper.h"

/**	 
 * @brief Initialize specific device of I2C filter
 *
 * @param i2c_filter_interface I2C filter interface to use
 *
 * @return 0 if the I2C filter failed to initialize or an error code.
 */
static int I2cFilterInit(struct i2c_filter_interface *filter)
{
	return WrapperI2cFilterInit(filter->filter.device_id);
}

/**	 
 * @brief Enable / disable specific device of I2C filter
 *
 * @param i2c_filter_interface I2C filter interface to use
 * @param en enable I2C filter
 *
 * @return 0 if the I2C filter failed to enable or an error code.
 */
static int I2cFilterEnable(struct i2c_filter_interface *filter, bool enable)
{
	return WrapperI2cFilterEnable(filter->filter.device_id, enable);
}

/**	 
 * @brief Setup whitelist for specific device of I2C filter with provided offset of slave device.
 * 			Index can using on stores every single offset of slave device with each single index.
 *
 * @param i2c_filter_interface I2C filter interface to use
 * @param index Index to stores register address or offset of slave device
 *
 * @return 0 if the I2C filter failed to setup or an error code.
 */
static int I2cFilterSet(struct i2c_filter_interface *filter, uint8_t index)
{
	return WrapperI2cFilterSet(filter->filter.device_id, index);
}

/**	 
 * @brief Retrieves a I2C filter interface provided functions. 
 *
 * @param i2c_filter_interface I2C filter interface to use
 *
 * @return 0 if the I2C filter failed to initialization or an error code.
 */
int I2cFilterInitialization(struct i2c_filter_interface *Filter)
{
	if (Filter == NULL)
		return -1;

	memset(Filter, 0, sizeof(*Filter));

	Filter->init_filter = I2cFilterInit;
	Filter->enable_filter = I2cFilterEnable;
	Filter->set_filter = I2cFilterSet;

	return 0;
}
