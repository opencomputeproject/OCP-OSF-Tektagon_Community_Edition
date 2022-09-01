/*
 * Copyright (c) 2021 Aspeed Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>
#include <tc_util.h>
#include <drivers/i2c.h>
#include <drivers/i2c/slave/eeprom.h>
#include <drivers/i2c/slave/ipmb.h>
#include "ast_test.h"
#include <random/rand32.h>

#define LOG_MODULE_NAME i2c_test

#include <logging/log.h>
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

#if DT_HAS_COMPAT_STATUS_OKAY(aspeed_i2c)
#define ASPEED_I2C_NUMBER 6
#define DT_DRV_COMPAT aspeed_i2c
#define DATA_COUNT 0x20
#define EEPROM_ADDR 0x40
#define IPMB_ADDR 0x50
#define RANDOM
#else
#error No known devicetree compatible match for I2C test
#endif

#define I2CMDRV "I2C_"
#define IPMBDRV "IPMB_SLAVE_"
#define EEPROMDRV "EEPROM_SLAVE_"

uint8_t i2c_speed[] = {I2C_SPEED_STANDARD,
					I2C_SPEED_FAST,
					I2C_SPEED_FAST_PLUS};

static void prepare_test_data(uint8_t *data, int nbytes)
{
	uint32_t value = sys_rand32_get();

#ifdef RANDOM
	uint32_t shift;

	for (int i = 0; i < nbytes; i++) {
		shift = (i & 0x3) * 8;
		data[i] = (value >> shift) & 0xff;
		if ((i & 0x3) == 0x3) {
			value = sys_rand32_get();
		}
	}
#else
	for (int i = 0; i < nbytes; i++) {
		data[i] = (value & 0xff) + i;
	}
#endif
}

void test_i2c_slave_EEPROM(void)
{
	int i, j, result;
	char name_m[20], name_s[20], num[10];
	uint8_t data_s[DATA_COUNT];
	uint8_t data_r[DATA_COUNT];
	const struct device *master_dev;
	const struct device *slave_dev;
	uint32_t dev_config_raw;
	uint32_t i2c_clock = I2C_SPEED_FAST;
	uint8_t dev_addr;

	/* change odd device as EEPROM slave device */
	for (i = 0; i < ASPEED_I2C_NUMBER ; i += 2) {
		dev_addr = EEPROM_ADDR + (i + 1);

		strcpy(name_m, I2CMDRV);
		sprintf(num, "%d", i);
		strcat(name_m, num);

		strcpy(name_s, EEPROMDRV);
		sprintf(num, "%d", (i+1));
		strcat(name_s, num);

		printk("I2C M : %d - EE S : %d\n", i, (i+1));

		/* obtain i2c master device */
		master_dev = device_get_binding(name_m);
		ast_zassert_not_null(master_dev, "I2C: %s Master device is got failed",
		name_m);

		/* change i2c speed */
		i2c_clock = i2c_speed[i%3];
		dev_config_raw = I2C_MODE_MASTER |
		I2C_SPEED_SET(i2c_clock);
		i2c_configure(master_dev, dev_config_raw);

		/* obtain i2c slave device */
		slave_dev = device_get_binding(name_s);
		ast_zassert_not_null(slave_dev, "I2C: %s Slave device is got failed",
		name_s);

		/* register i2c slave device */
		ast_zassert_false(i2c_slave_driver_register(slave_dev),
		"I2C: %s Slave register is got failed", name_s);

		/* fill transfer data */
		prepare_test_data(data_s, DATA_COUNT);

		/* burst transfer data */
		result = i2c_burst_write(master_dev, dev_addr, 0, data_s, DATA_COUNT);
		ast_zassert_false(result,
		"I2C: %s EEPROM write is got failed %d", name_m, result);

		/* burst receive data */
		result = i2c_burst_read(master_dev, dev_addr, 0, data_r, DATA_COUNT);
		ast_zassert_false(result,
		"I2C: %s EEPROM read is got failed %d", name_m, result);

		/* check data */
		for (j = 0; j < DATA_COUNT; j++) {
			ast_zassert_equal(data_s[j], data_r[j],
			"I2C: %s EEPROM R/W is got failed at %d, s: %d-r: %d",
			name_m, j, data_s[j], data_r[j]);
		}

		/* un-register i2c slave device */
		ast_zassert_false(i2c_slave_driver_unregister(slave_dev),
		"I2C: %s Slave un-register is got failed", name_s);

	}
}

void test_i2c_slave_IPMB(void)
{
	int i, j, result;
	char name_m[20], name_s[20], num[10];
	uint8_t data_s[DATA_COUNT];
	const struct device *master_dev;
	const struct device *slave_dev;
	uint32_t dev_config_raw;
	uint32_t i2c_clock = I2C_SPEED_FAST;
	uint8_t dev_addr;
	struct ipmb_msg *msg = NULL;
	uint8_t length = 0;
	uint8_t *buf = NULL;

	/* change even device as IPMB slave device */
	for (i = 0; i < ASPEED_I2C_NUMBER ; i += 2) {
		dev_addr = IPMB_ADDR + i;

		printk("I2C M : %d - IPMB S : %d:\n", (i+1), i);

		strcpy(name_m, I2CMDRV);
		sprintf(num, "%d", (i+1));
		strcat(name_m, num);

		strcpy(name_s, IPMBDRV);
		sprintf(num, "%d", i);
		strcat(name_s, num);

		/* obtain i2c master device */
		master_dev = device_get_binding(name_m);
		ast_zassert_not_null(master_dev, "I2C: %s Master device is got failed",
		name_m);

		/* change i2c speed */
		i2c_clock = i2c_speed[i%3];
		dev_config_raw = I2C_MODE_MASTER |
		I2C_SPEED_SET(i2c_clock);
		i2c_configure(master_dev, dev_config_raw);

		/* obtain i2c slave device */
		slave_dev = device_get_binding(name_s);
		ast_zassert_not_null(slave_dev, "I2C: %s Slave device is got failed",
		name_s);

		/* register i2c slave device */
		ast_zassert_false(i2c_slave_driver_register(slave_dev),
		"I2C: %s Slave register is got failed", name_s);

		/* fill transfer data */
		prepare_test_data(data_s, DATA_COUNT);

		/* burst transfer data */
		result = i2c_burst_write(master_dev, dev_addr, 0, data_s, DATA_COUNT);
		ast_zassert_false(result,
		"I2C: %s IPMB write is got failed %d", name_m, result);

		result = ipmb_slave_read(slave_dev, &msg, &length);
		ast_zassert_false(result,
		"I2C: %s IPMB read is got failed %d", name_s, result);

		if (!result) {
			/* check length */
			ast_zassert_equal(length, DATA_COUNT+2,
			"I2C: %s IPMB length is wrong %d ", name_m, length);

			buf = (uint8_t *)(msg);

			/* check length */
			ast_zassert_equal(buf[0], dev_addr << 1,
			"I2C: %s IPMB device id is wrong %d ", name_m, buf[0]);

			/* check message value */
			for (j = 2; j < length; j++) {
				ast_zassert_equal(data_s[j-2], buf[j],
				"I2C: %s IPMB R/W is got failed at %d, s: %d-r: %d",
				name_m, j, data_s[j-2], buf[j]);
			}
		}

		/* un-register i2c slave device */
		ast_zassert_false(i2c_slave_driver_unregister(slave_dev),
		"I2C: %s Slave un-register is got failed", name_s);

	}
}

int test_i2c(int count, enum aspeed_test_type type)
{
	printk("%s, count: %d, type: %d\n", __func__, count, type);

	for (int i = 0; i < count; i++) {
		printk("I2C slave EEPROM\n");
		test_i2c_slave_EEPROM();

		printk("I2C slave IPMB\n");
		test_i2c_slave_IPMB();
	}
	return ast_ztest_result();
}
