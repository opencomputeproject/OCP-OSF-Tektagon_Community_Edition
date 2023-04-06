#include <zephyr.h>
#include <device.h>
#include <drivers/i2c/pfr/i2c_filter.h>
#include <stdio.h>
#include <string.h>
#include "i2c_filter_aspeed.h"

static void i2c_filter_get_dev_name(uint8_t *string_buf, uint8_t filter_sel)
{
	uint8_t flt_num[I2C_FILTER_MIDDLEWARE_DEV_NAME_SIZE];

	memcpy(string_buf, I2C_FILTER_MIDDLEWARE_PREFIX, sizeof(I2C_FILTER_MIDDLEWARE_PREFIX));

	sprintf(flt_num, "%d", filter_sel);

	strcat(string_buf, flt_num);

	return;
}

static const struct device *i2c_filter_get_device(uint8_t filter_sel)
{
	const struct device *dev;
	uint8_t flt_name[I2C_FILTER_MIDDLEWARE_DEV_NAME_SIZE];

	i2c_filter_get_dev_name(flt_name, filter_sel);

	dev = device_get_binding(flt_name);

	if (!dev)
		printk("I2C PFR : FLT Device driver not found.");

	return dev;
}

/**
 * @brief Setup whitelist on specific slave address with provided offset and I2C filter device number.
 *
 * @param filter_sel Selection of I2C filter device
 * @param whitelist_tbl_idx table index to stores offset of slave device
 * @param slv_addr slave address
 * @param whitelist_tbl 256 bytes whitelist table
 *
 * @return 0 if the I2C filter failed to enable / disable or an error code.
 */
int i2c_filter_middleware_set_whitelist(uint8_t filter_sel, uint8_t whitelist_tbl_idx,
					uint8_t slv_addr, void *whitelist_tbl)
{
	const struct device *pfr_flt_dev;
	int ret;

	pfr_flt_dev = i2c_filter_get_device(filter_sel);

	if (!pfr_flt_dev)
		return -1;

	ret = ast_i2c_filter_update(pfr_flt_dev, whitelist_tbl_idx,
				    slv_addr, (struct ast_i2c_f_bitmap *)whitelist_tbl);

	if (ret)
		printk("I2C PFR : FLT Device Update failed.");
	return ret;
}

/**
 * @brief Enable /disable specific I2C filter device.
 *
 * @param filter_sel Selection of I2C filter device
 * @param en enable / disable I2C filter device
 *
 * @return 0 if the I2C filter failed to enable / disable or an error code.
 */
int i2c_filter_middleware_en(uint8_t filter_sel, bool en)
{
	const struct device *pfr_flt_dev;
	int ret;

	pfr_flt_dev = i2c_filter_get_device(filter_sel);

	if (!pfr_flt_dev)
		return -1;

	ret = ast_i2c_filter_en(pfr_flt_dev, en, 1, 0, 0);

	if (ret) {
		printk("I2C PFR : FLT Device %s failed.",
		       en ? "Enable" : "Disable");
	}

	return ret;
}

/**
 * @brief Initializes specific I2C filter device.
 *
 * @param filter_sel Selection of I2C filter device
 *
 * @return 0 if the I2C filter failed to initialization or an error code.
 */
int i2c_filter_middleware_init(uint8_t filter_sel)
{
	const struct device *pfr_flt_dev = NULL;
	int ret = 0;

	/* initial flt */
	pfr_flt_dev = i2c_filter_get_device(filter_sel);

	if (!pfr_flt_dev)
		return -1;

	ret = ast_i2c_filter_init(pfr_flt_dev);

	if (ret) {
		printk("I2C PFR : FLT Device Initial failed.");
		return ret;
	}

	ret = ast_i2c_filter_en(pfr_flt_dev, 1, 1, 0, 0);

	if (ret) {
		printk("I2C PFR : FLT Device Enable / Disable failed.");
		return ret;
	}

	ret = ast_i2c_filter_default(pfr_flt_dev, 0);

	if (ret) {
		printk("I2C PFR : FLT Device Set Default failed.");
		return ret;
	}

	return ret;
}
