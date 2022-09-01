/*
 * Copyright (c) 2021 Aspeed Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_DRIVERS_I2C_MAILBOX_H_
#define ZEPHYR_INCLUDE_DRIVERS_I2C_MAILBOX_H_

/* mail box define */
#define AST_I2C_M_DEV_COUNT		2
#define AST_I2C_M_WP_COUNT		8
#define AST_I2C_M_IRQ_COUNT		16

#define AST_I2C_M_R		1
#define AST_I2C_M_W		2

#define AST_I2C_M_I2C1	1
#define AST_I2C_M_I2C2	2

/* mail box fifo define */
#define AST_I2C_M_FIFO_COUNT	2
#define AST_I2C_M_FIFO_REMAP	BIT(20)
#define AST_I2C_M_FIFO0_MASK	0xFFFCFF00
#define AST_I2C_M_FIFO0_INT_EN	(BIT(16) | BIT(17))
#define AST_I2C_M_FIFO0_INT_EN	(BIT(16) | BIT(17))
#define AST_I2C_M_FIFO0_W_DIS	BIT(16)
#define AST_I2C_M_FIFO0_R_DIS	BIT(17)
#define AST_I2C_M_FIFO0_INT_STS	(BIT(0) | BIT(1))
#define AST_I2C_M_FIFO1_MASK	0xFFF300FF
#define AST_I2C_M_FIFO1_INT_EN	(BIT(18) | BIT(19))
#define AST_I2C_M_FIFO1_W_DIS	BIT(18)
#define AST_I2C_M_FIFO1_R_DIS	BIT(19)
#define AST_I2C_M_FIFO1_INT_STS	(BIT(2) | BIT(3))
#define AST_I2C_M_FIFO_R_NAK	BIT(28)
#define AST_I2C_M_FIFO_INT	0xF0000
#define AST_I2C_M_FIFO_STS	0xF
#define AST_I2C_M_FIFO_I2C1_H	BIT(24)
#define AST_I2C_M_FIFO_I2C2_H	BIT(25)

/* mail box base define */
#define AST_I2C_M_BASE			0x3000

/* dedicated mailbox base and length*/
#define AST_I2C_M_OFFSET		0x200
#define AST_I2C_M_LENGTH		0x100

/* i2c device registers */
#define AST_I2C_CTL				0x00
#define AST_I2C_S_IER				0x20
#define AST_I2C_DMA_LEN			0x2C
#define AST_I2C_TX_DMA			0x38
#define AST_I2C_RX_DMA			0x3C
#define AST_I2C_ADDR			0x40
#define AST_I2C_MBX_LIM			0x70

/* device registers */
#define AST_I2C_M_CFG			0x04
#define AST_I2C_M_FIFO_IRQ		0x08
#define AST_I2C_M_IRQ_EN0		0x10
#define AST_I2C_M_IRQ_STA0		0x14
#define AST_I2C_M_IRQ_EN1		0x18
#define AST_I2C_M_IRQ_STA1		0x1C

/* (0) 0x20 ~ 0x3C /(1)0x40 ~ 0x5C */
#define AST_I2C_M_WP_BASE0		0x20
#define AST_I2C_M_WP_BASE1		0x40

#define AST_I2C_M_FIFO_CFG0		0x60
#define AST_I2C_M_FIFO_STA0		0x64
#define AST_I2C_M_FIFO_CFG1		0x68
#define AST_I2C_M_FIFO_STA1		0x6C
#define AST_I2C_M_ADDR_IRQ0		0x70
#define AST_I2C_M_ADDR_IRQ1		0x74
#define AST_I2C_M_ADDR_IRQ2		0x78
#define AST_I2C_M_ADDR_IRQ3		0x7C

#define AST_I2C_MASTER_EN		BIT(0)
#define AST_I2C_SLAVE_EN		BIT(1)
#define AST_I2C_MBX_EN			(BIT(23) | BIT(24))

/* i2c slave address control */
#define AST_I2CS_ADDR3_MBX_TYPE(x)	(x << 28)
#define AST_I2CS_ADDR2_MBX_TYPE(x)	(x << 26)
#define AST_I2CS_ADDR1_MBX_TYPE(x)	(x << 24)
#define AST_I2CS_ADDR3_ENABLE		BIT(23)
#define AST_I2CS_ADDR3(x)			((x & 0x7f) << 16)
#define AST_I2CS_ADDR2_ENABLE		BIT(15)
#define AST_I2CS_ADDR2(x)			((x & 0x7f) << 8)
#define AST_I2CS_ADDR1_ENABLE		BIT(7)
#define AST_I2CS_ADDR1(x)			(x & 0x7f)

#define	AST_I2CS_ADDR3_MASK	(0x7f << 16)
#define	AST_I2CS_ADDR2_MASK	(0x7f << 8)
#define	AST_I2CS_ADDR1_MASK	0x7f

#define	AST_I2CS_ADDR3_CLEAR	(AST_I2CS_ADDR3_MASK |\
AST_I2CS_ADDR3_ENABLE|AST_I2CS_ADDR3_MBX_TYPE(0x3))

#define	AST_I2CS_ADDR2_CLEAR	(AST_I2CS_ADDR2_MASK |\
AST_I2CS_ADDR2_ENABLE|AST_I2CS_ADDR2_MBX_TYPE(0x3))

#define	AST_I2CS_ADDR1_CLEAR	(AST_I2CS_ADDR1_MASK |\
AST_I2CS_ADDR1_ENABLE|AST_I2CS_ADDR1_MBX_TYPE(0x3))

/* i2c slave address control */
#define AST_I2C_MBX_RX_DMA_LEN(x)	\
((((x - 1) & 0xfff) << 16) | BIT(31))

#define AST_I2C_MBX_TX_DMA_LEN(x)	\
(((x - 1) & 0xfff) | BIT(15))

/* i2c fifo internal base and length limitation */
#define FIFO_MAX_SIZE 0x100
#define FIFO_BASE 0x300

/* i2c fifo cpu access base */
#define FIFO0_ACCESS 0x1f00
#define FIFO1_ACCESS 0x1f04

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initial i2c mailbox device
 *
 * @param dev Pointer to the device structure for the driver instance
 *
 * @retval 0 If successful
 * @retval -EINVAL Invalid data pointer or offset
 */
int ast_i2c_mbx_init(const struct device *dev);

/**
 * @brief Set i2c mailbox device slave address
 *
 * @param dev Pointer to the device structure for the driver instance
 * @param dev_idx Index to the mailbox device index
 * @param idx Index to the slave address
 * @param offset Offset to the slave address length
 * @param addr Address to the mbx device
 * @param enable Enable flag to the mbx device
 *
 * @retval 0 If successful
 * @retval -EINVAL Invalid data pointer or offset
 */
int ast_i2c_mbx_addr(const struct device *dev, uint8_t dev_idx,
uint8_t idx, uint8_t offset, uint8_t addr, uint8_t enable);

/**
 * @brief Enable i2c mailbox device
 *
 * @param dev Pointer to the device structure for the driver instance
 * @param dev_idx Index to the mailbox device index
 * @param enable Enable flag to the mbx device
 *
 * @retval 0 If successful
 * @retval -EINVAL Invalid data pointer or offset
 */
int ast_i2c_mbx_en(const struct device *dev, uint8_t dev_idx,
uint8_t enable);

/**
 * @brief Set i2c mailbox write protect index
 *
 * @param dev Pointer to the device structure for the driver instance
 * @param dev_idx Index to the mailbox device index
 * @param addr Address to the write protect in mbx device
 * @param enable Enable flag to the write protect in mbx device
 *
 * @retval 0 If successful
 * @retval -EINVAL Invalid data pointer or offset
 */
int ast_i2c_mbx_protect(const struct device *dev, uint8_t dev_idx,
uint8_t addr, uint8_t enable);

/**
 * @brief Set i2c mailbox notify address
 *
 * @param dev Pointer to the device structure for the driver instance
 * @param idx Index to the notify internal index position (0x0~0x15)
 * @param addr Address to the write notify in mbx device
 *
 * @retval 0 If successful
 * @retval -EINVAL Invalid data pointer or offset
 */
int ast_i2c_mbx_notify_addr(const struct device *dev, uint8_t idx,
uint8_t addr);

/**
 * @brief Set i2c mailbox notify enable
 *
 * @param dev Pointer to the device structure for the driver instance
 * @param dev_idx Index to the mailbox device index
 * @param idx Index to the notify internal index position (0x0~0x15)
 * @param type Type (read / write) to the notify in mbx device
 * @param enable Enable flag to the notify
 *
 * @retval 0 If successful
 * @retval -EINVAL Invalid data pointer or offset
 */
int ast_i2c_mbx_notify_en(const struct device *dev, uint8_t dev_idx,
uint8_t idx, uint8_t type, uint8_t enable);

/**
 * @brief Set i2c mailbox fifo enable
 *
 * @param dev Pointer to the device structure for the driver instance
 * @param idx Index to the fifo  (0x0~0x1)
 * @param base Buffer base to the fifo
 * @param length Buffer length to the fifo (0 means close fifo)
 *
 * @retval 0 If successful
 * @retval -EINVAL Invalid data pointer or offset
 */
int ast_i2c_mbx_fifo_en(const struct device *dev, uint8_t idx,
uint16_t base, uint16_t length);

/**
 * @brief Set i2c mailbox fifo apply
 *
 * @param dev Pointer to the device structure for the driver instance
 * @param idx Index to the fifo  (0x0~0x1)
 * @param addr fifo address to the mbx
 * @param type Type disable (read / write) to the fifo in mbx device
 *
 * @retval 0 If successful
 * @retval -EINVAL Invalid data pointer or offset
 */
int ast_i2c_mbx_fifo_apply(const struct device *dev, uint8_t idx,
uint8_t addr, uint8_t type);

/**
 * @brief Set i2c mailbox fifo priority
 *
 * @param dev Pointer to the device structure for the driver instance
 * @param pirority I2C access priority (0x0~0x2) 0x0 means no pirority setting
 *
 * @retval 0 If successful
 * @retval -EINVAL Invalid data pointer or offset
 */
int ast_i2c_mbx_fifo_priority(const struct device *dev, uint8_t priority);

/**
 * @brief Do access mbx fifo from CPU
 *
 * @param dev Pointer to the device structure for the driver instance
 * @param idx Index to the fifo  (0x0~0x1)
 * @param type Index to the access type (Read: 0x1 / Write:0x2)
 * @param data to/from the mbx fifo
 *
 * @retval 0 If successful
 * @retval -EINVAL Invalid data pointer or offset
 */
int ast_i2c_mbx_fifo_access(const struct device *dev, uint8_t idx,
uint8_t type, uint8_t *data);

/**
 * @}
 */
#ifdef __cplusplus
	}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_I2C_MAILBOX_H_ */
