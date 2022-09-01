/*
 * Copyright (c) 2021 Aspeed Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
*/
#ifndef ZEPHYR_INCLUDE_DRIVERS_I2C_SLAVE_IPMB_H_
#define ZEPHYR_INCLUDE_DRIVERS_I2C_SLAVE_IPMB_H_

/* ipmb define*/
#define IPMB_REQUEST_LEN	0x7
#define IPMB_TOTAL_LEN		0x100

#define GET_ADDR(addr)	((addr << 1) & 0xff)
#define LIST_SIZE sizeof(sys_snode_t)
#define IPMB_MSG_DATA_LEN (IPMB_TOTAL_LEN - IPMB_REQUEST_LEN - LIST_SIZE - 1)

struct ipmb_msg {
	uint8_t rsSA;
	uint8_t netFn_rsLUN;
	uint8_t checksum;
	uint8_t rqSA;
	uint8_t rqSeq_rqLUN;
	uint8_t cmd;
	uint8_t data_bytes[IPMB_MSG_DATA_LEN];
} __packed;

/**
 * @brief I2C IPMB Slave Driver API
 * @defgroup i2c_ipmb_slave_api i2c IPMB slave driver API
 * @ingroup io_interfaces
 * @{
 */

/**
 * @brief Read single buffer of virtual IPMB memory
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param ipmb_data Pointer of byte where to store the virtual ipmb memory
 * @param length Pointer of byte where to store the length of ipmb message
 * @retval 0 If successful
 * @retval -EINVAL Invalid data pointer or offset
 */
int ipmb_slave_read(const struct device *dev, struct ipmb_msg **ipmb_data, uint8_t *length);

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_DRIVERS_I2C_SLAVE_IPMB_H_ */
