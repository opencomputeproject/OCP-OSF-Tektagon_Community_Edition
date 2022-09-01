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

#ifndef ZEPHYR_DRIVERS_CRYPTO_RSA_ASPEED_PRIV_H_
#define ZEPHYR_DRIVERS_CRYPTO_RSA_ASPEED_PRIV_H_

#include <kernel.h>
#include <crypto/rsa_structs.h>

struct aspeed_rsa_ctx {
	struct rsa_key key;
	uint32_t len;
};

struct aspeed_rsa_drv_state {
	struct aspeed_rsa_ctx data;
	uint8_t sram;
	bool in_use;
};

#endif  /* ZEPHYR_DRIVERS_CRYPTO_RSA_ASPEED_PRIV_H_ */
