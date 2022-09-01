/*
 * Copyright (c) 2021 ASPEED Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Crypto Hash structure definitions
 *
 * This file contains the Hash Abstraction layer structures.
 *
 * [Experimental] Users should note that the Structures can change
 * as a part of ongoing development.
 */

#ifndef ZEPHYR_INCLUDE_CRYPTO_HASH_STRUCTS_H_
#define ZEPHYR_INCLUDE_CRYPTO_HASH_STRUCTS_H_

#include <device.h>
#include <sys/util.h>
/**
 * @addtogroup crytpo_hash
 * @{
 */


/** Hash Algorithm. */
enum hash_algo {
	HASH_SHA256 = 1,
	HASH_SHA384 = 2,
	HASH_SHA512 = 3,
};

/* Forward declarations */
struct hash_ctx;
struct hash_pkt;


typedef int (*hash_update_t)(struct hash_ctx *ctx, struct hash_pkt *pkt);
typedef int (*hash_final_t)(struct hash_ctx *ctx, struct hash_pkt *pkt);

struct hash_ops {
	hash_update_t update_hndlr;
	hash_final_t final_hndlr;
};

/**
 * Structure encoding session parameters.
 *
 * Refer to comments for individual fields to know the contract
 * in terms of who fills what and when w.r.t begin_session() call.
 */
struct hash_ctx {

	/** Place for driver to return function pointers to be invoked per
	 * cipher operation. To be populated by crypto driver on return from
	 * begin_session() based on the algo/mode chosen by the app.
	 */
	struct hash_ops ops;

	/** The device driver instance this crypto context relates to. Will be
	 * populated by the begin_session() API.
	 */
	const struct device *device;

	/** If the driver supports multiple simultaneously crypto sessions, this
	 * will identify the specific driver state this crypto session relates
	 * to. Since dynamic memory allocation is not possible, it is
	 * suggested that at build time drivers allocate space for the
	 * max simultaneous sessions they intend to support. To be populated
	 * by the driver on return from begin_session().
	 */
	void *drv_sessn_state;

	/** Place for the user app to put info relevant stuff for resuming when
	 * completion callback happens for async ops. Totally managed by the
	 * app.
	 */
	void *app_sessn_state;

	uint32_t digest_size;
};

/**
 * Structure encoding IO parameters of one cryptographic
 * operation like encrypt/decrypt.
 *
 * The fields which has not been explicitly called out has to
 * be filled up by the app before making the cipher_xxx_op()
 * call.
 */
struct hash_pkt {

	/** Start address of input buffer */
	uint8_t *in_buf;

	/** Bytes to be operated upon */
	int  in_len;

	/** Start of the output buffer, to be allocated by
	 * the application. Can be NULL for in-place ops. To be populated
	 * with contents by the driver on return from op / async callback.
	 */
	uint8_t *out_buf;

	/** Size of the out_buf area allocated by the application. Drivers
	 * should not write past the size of output buffer.
	 */
	int out_buf_max;

	/** To be populated by driver on return from cipher_xxx_op() and
	 * holds the size of the actual result.
	 */
	int out_len;

	/** Context this packet relates to. This can be useful to get the
	 * session details, especially for async ops. Will be populated by the
	 * cipher_xxx_op() API based on the ctx parameter.
	 */
	struct hash_ctx *ctx;
};

/**
 * @}
 */
#endif /* ZEPHYR_INCLUDE_CRYPTO_HASH_STRUCTS_H_ */
