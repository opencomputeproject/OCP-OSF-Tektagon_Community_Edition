#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include <i2c/i2c_filter.h>
#include "I2cFilter/I2cFilter.h"

static int WrapperI2cFilterInit(struct i2c_filter_interface *filter);
static int WrapperI2cFilterEnable(struct i2c_filter_interface *filter, bool enable);
static int WrapperI2cFilterSet(struct i2c_filter_interface *filter, uint8_t index);