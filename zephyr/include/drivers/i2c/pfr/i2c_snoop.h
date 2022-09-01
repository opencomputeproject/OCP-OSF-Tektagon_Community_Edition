/*
 * Copyright (c) 2021 Aspeed Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_DRIVERS_I2C_SNOOP_H_
#define ZEPHYR_INCLUDE_DRIVERS_I2C_SNOOP_H_

/* snoop define */
#define AST_I2C_SP_DEV_COUNT	2
#define AST_I2C_SP_MSG_COUNT	1024
#define AST_I2C_F_COUNT			5

#define AST_I2C_SP_LOOP			BIT(12)
#define AST_I2C_SP_EN			BIT(11)
#define AST_I2C_PKT_MODE_EN	BIT(16)
#define AST_I2C_RX_DMA_EN		BIT(9)

#define AST_I2C_SP_CMD	\
(AST_I2C_SP_LOOP | AST_I2C_SP_EN | AST_I2C_PKT_MODE_EN\
| AST_I2C_RX_DMA_EN)

#define AST_I2C_ADDR_TYPE(x)	(x << 24)
#define AST_I2C_ADDR_ENABLE	BIT(7)
#define AST_I2C_SP_ADDR(x)	(x & 0x7f)

#define	AST_I2C_ADDR_MASK	0x7f

#define	AST_I2CS_ADDR_CLEAR		\
(AST_I2C_ADDR_MASK | AST_I2C_ADDR_ENABLE |\
AST_I2C_ADDR_TYPE(0x3))

#define AST_I2CS_SET_RX_DMA_LEN(x)\
((((x - 1) & 0xfff) << 16) | BIT(31))

#define AST_I2C_S_EN	BIT(1)
#define AST_I2C_M_EN	BIT(0)

/* register define */
#define  AST_I2C_CTRL		0x00
#define  AST_I2CS_IER			0x20
#define  AST_I2CS_CMD		0x28
#define  AST_I2C_RX_DMA_LEN	0x2C
#define  AST_I2C_RX_DMA_BUF	0x3C
#define AST_I2C_ADDR_CTRL	0x40
#define  AST_I2C_SP_DMA_WPT		0x50
#define  AST_I2C_SP_DMA_RPT		0x58
#define  AST_I2C_SP_R_DMA_WPT	0x5C

#define  AST_I2C_SP_SWITCH	0x7e6e20f8

#define  AST_SP0_BASE		0x7e7b0780
#define  AST_SP1_BASE		0x7e7b0800

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief I2C Snoop Driver API
 * @{
 */
/**
 * @brief update i2c snoop buffer
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param size Value to the read back size.
 * @retval 0 If successful.
 * @retval -EINVAL Invalid data pointer or offset
 */
int ast_i2c_snoop_update(const struct device *dev, uint32_t size);

/**
 * @brief enable i2c snoop device
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param snoop_en Value to the snoop device enable.
 * @param filter_idx Value to the filter device index.
 * @param addr Value to the device address that is needed snoop.
 * @retval 0 If successful.
 * @retval -EINVAL Invalid data pointer or offset
 */
int ast_i2c_snoop_en(const struct device *dev, uint8_t snoop_en,
uint8_t filter_idx, uint8_t addr);


/**
 * @brief Initial i2c snoop device
 *
 * @param dev Pointer to the device structure for the driver instance.
 *
 * @retval 0 If successful.
 * @retval -EINVAL Invalid data pointer or offset
 */
int ast_i2c_snoop_init(const struct device *dev);
/**
 * @}
 */
#ifdef __cplusplus
	}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_I2C_SNOOP_H_ */
