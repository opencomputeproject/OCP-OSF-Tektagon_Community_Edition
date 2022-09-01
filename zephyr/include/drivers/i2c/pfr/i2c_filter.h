/*
 * Copyright (c) 2021 Aspeed Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_DRIVERS_I2C_FILTER_H_
#define ZEPHYR_INCLUDE_DRIVERS_I2C_FILTER_H_

/* filter capability define */
#define AST_I2C_F_COUNT			5
#define AST_I2C_F_REMAP_SIZE	16
#define AST_I2C_F_ELEMENT_SIZE	8
/* base on the pclk clock frequency */
#define AST_I2C_F_100_TIMING_VAL	0x01f400fa
#define AST_I2C_F_400_TIMING_VAL	0x007d003e

/* global define */
#define AST_I2C_F_G_BASE		0x2000

/* global registers */
#define AST_I2C_F_G_INT_EN	0x08
#define AST_I2C_F_G_INT_STS	0x0C

/* device define */
#define AST_I2C_F_D_BASE		0x200
#define AST_I2C_F_D_OFFSET	0x100

/* device registers */
#define AST_I2C_F_EN			0x04
#define AST_I2C_F_BUF		0x08
#define AST_I2C_F_CFG		0x0C
#define AST_I2C_F_TIMING		0x10
#define AST_I2C_F_INT_EN		0x14
#define AST_I2C_F_INT_STS	0x18
#define AST_I2C_F_STS		0x20
#define AST_I2C_F_MAP0		0x40
#define AST_I2C_F_MAP1		0x44
#define AST_I2C_F_MAP2		0x48
#define AST_I2C_F_MAP3		0x4C
#define AST_I2C_F_INFO		0x60

#ifdef __cplusplus
extern "C" {
#endif

struct ast_i2c_f_bitmap {
	uint32_t		element[AST_I2C_F_ELEMENT_SIZE];
};

struct ast_i2c_f_tbl {
	struct ast_i2c_f_bitmap	filter_tbl[AST_I2C_F_REMAP_SIZE+1];
};

/**
 * @brief I2C Filter Driver API
 * @{
 */
/**
 * @brief i2c filter ISR
 *
 * @param dev Pointer to the device structure for the driver instance.
 *
 * @retval 0 If successful.
 */
void ast_i2c_filter_isr(const struct device *dev);

/**
 * @brief set i2c filter default handle behavior
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param pass Value to the default white list behavior.
 * @retval 0 If successful.
 * @retval -EINVAL Invalid data pointer or offset
 */
int ast_i2c_filter_default(const struct device *dev, uint8_t pass);

/**
 * @brief update i2c filter device
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param idx Value to the index of re-map table.
 * @param addr Value to the white list address.
 * @param table Pointer to the filter bitmap table.
 * @retval 0 If successful.
 * @retval -EINVAL Invalid data pointer or offset
 */
int ast_i2c_filter_update(const struct device *dev, uint8_t idx, uint8_t addr,
struct ast_i2c_f_bitmap *table);

/**
 * @brief enable i2c filter device
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param filter_en Value to the filter device enable.
 * @param wlist_en Value to the white list enable.
 * @param clr_idx Value to the clear index.
 * @param clr_tbl Value to the clear white list table.
 * @retval 0 If successful.
 * @retval -EINVAL Invalid data pointer or offset
 */
int ast_i2c_filter_en(const struct device *dev, uint8_t filter_en, uint8_t wlist_en,
uint8_t clr_idx, uint8_t clr_tbl);

/**
 * @brief Initial i2c filter device
 *
 * @param dev Pointer to the device structure for the driver instance.
 *
 * @retval 0 If successful.
 * @retval -EINVAL Invalid device pointer
 */
int ast_i2c_filter_init(const struct device *dev);
/**
 * @}
 */
#ifdef __cplusplus
	}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_I2C_FILTER_H_ */
