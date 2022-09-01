/*
 * Copyright (c) 2021 ASPEED Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Crypto ECDSA structure definitions
 *
 * This file contains the ECDSA Abstraction layer structures.
 *
 * [Experimental] Users should note that the Structures can change
 * as a part of ongoing development.
 */

#ifndef ZEPHYR_INCLUDE_CRYPTO_ECDSA_STRUCTS_H_
#define ZEPHYR_INCLUDE_CRYPTO_ECDSA_STRUCTS_H_

#include <device.h>
#include <sys/util.h>

/* Curves IDs */
#define ECC_CURVE_NIST_P192	0x0001
#define ECC_CURVE_NIST_P256	0x0002
#define ECC_CURVE_NIST_P384	0x0003

/**
 * @addtogroup crytpo_ecdsa
 * @{
 */

/* Forward declarations */
struct ecdsa_ctx;
struct ecdsa_pkt;

struct ecdsa_key {
	unsigned int curve_id;
	char *qx;
	char *qy;
	char *d;
};

struct ecdsa_ops {
	int (*sign)(struct ecdsa_ctx *ctx, struct ecdsa_pkt *pkt);
	int (*verify)(struct ecdsa_ctx *ctx, struct ecdsa_pkt *pkt);
};

/**
 * Structure encoding session parameters.
 *
 * Refer to comments for individual fields to know the contract
 * in terms of who fills what and when w.r.t begin_session() call.
 */
struct ecdsa_ctx {

	/** Place for driver to return function pointers to be invoked per
	 * cipher operation. To be populated by crypto driver on return from
	 * begin_session() based on the algo/mode chosen by the app.
	 */
	struct ecdsa_ops ops;

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
};

/**
 * Structure encoding IO parameters of one cryptographic
 * operation like sign/verify.
 *
 * The fields which has not been explicitly called out has to
 * be filled up by the app before making the cipher_xxx_op()
 * call.
 */
struct ecdsa_pkt {

	uint8_t *m;
	int  m_len;

	uint8_t *r;
	int  r_len;

	uint8_t *s;
	int  s_len;

	/** Context this packet relates to. This can be useful to get the
	 * session details, especially for async ops. Will be populated by the
	 * cipher_xxx_op() API based on the ctx parameter.
	 */
	struct ecdsa_ctx *ctx;
};

/**
 * @}
 */
#endif /* ZEPHYR_INCLUDE_CRYPTO_ECDSA_STRUCTS_H_ */
