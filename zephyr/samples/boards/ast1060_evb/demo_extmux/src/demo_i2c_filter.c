/*
 * Copyright (c) 2021 ASPEED Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <sys/printk.h>
#include <drivers/i2c.h>
#include <drivers/i2c/pfr/i2c_filter.h>

#if CONFIG_I2C_PFR_FILTER
static struct ast_i2c_f_bitmap data_flt[] = {
{
{	/* block all (index 0) */
	0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000
}
},
{
{	/* accept all (index 1) */
	0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
	0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF
}
},
{
{	/* block every 16 byte (index 2) */
	0xFFFF0000, 0xFFFF0000, 0xFFFF0000, 0xFFFF0000,
	0xFFFF0000, 0xFFFF0000, 0xFFFF0000, 0xFFFF0000
}
},
{
{	/* block first 16 byte (index 3) */
	0xFFFF0000, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
	0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF
}
},
{
{	/* block first 128 byte (index 4) */
	0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF
}
},
{
{	/* block last 128 byte (index 5) */
	0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
	0x00000000, 0x00000000, 0x00000000, 0x00000000,
}
}
};
#endif

void ast1060_i2c_demo_flt(void)
{
#if CONFIG_I2C_PFR_FILTER
	uint8_t EEPROM_COUNT = 8;
	uint8_t EEPROM_PASS_TBL[] = {0, 0, 0, 1, 1, 4, 4, 5};
	const struct device *pfr_flt_dev = NULL;
	int ret = 0, i = 0;

	/* initial flt */
	pfr_flt_dev = device_get_binding("I2C_FILTER_0");
	if (!pfr_flt_dev) {
		printk("I2C PFR : FLT Device driver not found.");
		return;
	}

	if (pfr_flt_dev != NULL) {
		ret = ast_i2c_filter_init(pfr_flt_dev);
		if (ret) {
			printk("I2C PFR : FLT Device Initial failed.");
			return;
		}

		ret = ast_i2c_filter_en(pfr_flt_dev, 1, 1, 0, 0);
		if (ret) {
			printk("I2C PFR : FLT Device Enable / Disable failed.");
			return;
		}

		ret = ast_i2c_filter_default(pfr_flt_dev, 0);
		if (ret) {
			printk("I2C PFR : FLT Device Set Default failed.");
			return;
		}

		for (i = 0x0; i < EEPROM_COUNT; i++) {
			ret = ast_i2c_filter_update(pfr_flt_dev, (uint8_t)(i),
			(uint8_t) (0x50 + i), &data_flt[EEPROM_PASS_TBL[i]]);
			if (ret) {
				printk("I2C PFR : FLT Device Update failed.");
				return;
			}
		}
	}
#endif
}

