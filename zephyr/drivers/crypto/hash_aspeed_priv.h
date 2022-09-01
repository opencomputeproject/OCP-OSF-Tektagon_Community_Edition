/*
 * Copyright (c) 2021 ASPEED Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief tinycrypt driver context info
 *
 * The file defines the structure which is used to store per session context
 * by the driver. Placed in common location so that crypto applications
 * can allocate memory for the required number of sessions, to free driver
 * from dynamic memory allocation.
 */

#ifndef ZEPHYR_DRIVERS_CRYPTO_HASH_ASPEED_PRIV_H_
#define ZEPHYR_DRIVERS_CRYPTO_HASH_ASPEED_PRIV_H_

#include <kernel.h>

#define ASPEED_HACE_STS					0x1C
#define  HACE_RSA_ISR					BIT(13)
#define  HACE_CRYPTO_ISR				BIT(12)
#define  HACE_HASH_ISR					BIT(9)
#define  HACE_RSA_BUSY					BIT(2)
#define  HACE_CRYPTO_BUSY				BIT(1)
#define  HACE_HASH_BUSY					BIT(0)
#define ASPEED_HACE_HASH_SRC			0x20
#define ASPEED_HACE_HASH_DIGEST_BUFF	0x24
#define ASPEED_HACE_HASH_KEY_BUFF		0x28
#define ASPEED_HACE_HASH_DATA_LEN		0x2C
#define  HACE_SG_LAST					BIT(31)
#define ASPEED_HACE_HASH_CMD			0x30
#define  HACE_SHA_BE_EN					BIT(3)
#define  HACE_MD5_LE_EN					BIT(2)
#define  HACE_ALGO_MD5					0
#define  HACE_ALGO_SHA1					BIT(5)
#define  HACE_ALGO_SHA224				BIT(6)
#define  HACE_ALGO_SHA256				(BIT(4) | BIT(6))
#define  HACE_ALGO_SHA512				(BIT(5) | BIT(6))
#define  HACE_ALGO_SHA384				(BIT(5) | BIT(6) | BIT(10))
#define  HASH_CMD_ACC_MODE				(0x2 << 7)
#define  HACE_SG_EN						BIT(18)

struct aspeed_sg {
	uint32_t len;
	uint32_t addr;
};

struct aspeed_hash_ctx {
	struct aspeed_sg sg[2]; /* Must be 8 byte aligned */
	uint8_t digest[64]; /* Must be 8 byte aligned */
	uint32_t method;
	uint32_t block_size;
	uint64_t digcnt[2]; /* total length */
	uint32_t bufcnt;
	uint8_t buffer[256];
};

struct aspeed_hash_drv_state {
	struct aspeed_hash_ctx data;
	bool in_use;
};

#endif  /* ZEPHYR_DRIVERS_CRYPTO_HASH_ASPEED_PRIV_H_ */
