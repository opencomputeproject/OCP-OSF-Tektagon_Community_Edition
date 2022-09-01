/*
 * Copyright (c) 2016 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief RSA APIs
 *
 * This file contains the RSA Abstraction layer APIs.
 *
 * [Experimental] Users should note that the APIs can change
 * as a part of ongoing development.
 */

/**
 * @brief Crypto APIs
 * @defgroup crypto RSA
 * @{
 * @}
 */

/**
 * @brief Crypto RSA APIs
 * @defgroup crypto_rsa RSA
 * @ingroup crypto
 * @{
 */

#ifndef ZEPHYR_INCLUDE_CRYPTO_RSA_H_
#define ZEPHYR_INCLUDE_CRYPTO_RSA_H_

#include <device.h>
#include <errno.h>
#include <sys/util.h>
#include <sys/__assert.h>
#include "rsa_structs.h"

/* The API a rsa driver should implement */
__subsystem struct rsa_driver_api {
	int (*query_hw_caps)(const struct device *dev);

	/* Setup a rsa session */
	int (*begin_session)(const struct device *dev, struct rsa_ctx *ctx,
						 struct rsa_key *key);

	/* Tear down an established session */
	int (*free_session)(const struct device *dev, struct rsa_ctx *ctx);
};

/**
 * @brief Query the rsa hardware capabilities
 *
 * This API is used by the app to query the capabilities supported by the
 * rsa device. Based on this the app can specify a subset of the supported
 * options to be honored for a session during rsa_begin_session().
 *
 * @param dev Pointer to the device structure for the driver instance.
 *
 * @return bitmask of supported options.
 */
static inline int rsa_query_hwcaps(const struct device *dev)
{
	struct rsa_driver_api *api;
	int tmp;

	api = (struct rsa_driver_api *)dev->api;

	tmp = api->query_hw_caps(dev);

	return tmp;
}

/**
 * @brief Setup a rsa session
 *
 * Initializes one time parameters, like the session key, algorithm and cipher
 * mode which may remain constant for all operations in the session. The state
 * may be cached in hardware and/or driver data state variables.
 *
 * @param  dev      Pointer to the device structure for the driver instance.
 * @param  ctx      Pointer to the context structure.
 *
 * @return 0 on success, negative errno code on fail.
 */
static inline int rsa_begin_session(const struct device *dev,
									struct rsa_ctx *ctx, struct rsa_key *key)
{
	struct rsa_driver_api *api;

	api = (struct rsa_driver_api *)dev->api;
	ctx->device = dev;

	return api->begin_session(dev, ctx, key);
}

/**
 * @brief Cleanup a rsa session
 *
 * Clears the hardware and/or driver state of a previous session.
 *
 * @param  dev      Pointer to the device structure for the driver instance.
 * @param  ctx      Pointer to the rsa context structure of the session
 *			to be freed.
 *
 * @return 0 on success, negative errno code on fail.
 */
static inline int rsa_free_session(const struct device *dev,
								   struct rsa_ctx *ctx)
{
	struct rsa_driver_api *api;

	api = (struct rsa_driver_api *)dev->api;

	return api->free_session(dev, ctx);
}

/**
 * @brief Perform RSA encrypt.
 *
 * @param  ctx   Pointer to the rsa context of this op.
 * @param  pkt   Structure holding the input/output buffer pointers.
 *
 * @return 0 on success, negative errno code on fail.
 */
static inline int rsa_encrypt(struct rsa_ctx *ctx,
							  struct rsa_pkt *pkt)
{
	pkt->ctx = ctx;
	return ctx->ops.encrypt(ctx, pkt);
}

/**
 * @brief Perform RSA encrypt.
 *
 * @param  ctx   Pointer to the rsa context of this op.
 * @param  pkt   Structure holding the input/output buffer pointers.
 *
 * @return 0 on success, negative errno code on fail.
 */
static inline int rsa_decrypt(struct rsa_ctx *ctx,
							  struct rsa_pkt *pkt)
{
	pkt->ctx = ctx;
	return ctx->ops.decrypt(ctx, pkt);
}

/**
 * @brief Perform RSA sign.
 *
 * @param  ctx   Pointer to the rsa context of this op.
 * @param  pkt   Structure holding the input/output buffer pointers.
 *
 * @return 0 on success, negative errno code on fail.
 */
static inline int rsa_sign(struct rsa_ctx *ctx,
						   struct rsa_pkt *pkt)
{
	pkt->ctx = ctx;
	return ctx->ops.sign(ctx, pkt);
}

/**
 * @brief Perform RSA sign.
 *
 * @param  ctx   Pointer to the rsa context of this op.
 * @param  pkt   Structure holding the input/output buffer pointers.
 *
 * @return 0 on success, negative errno code on fail.
 */
static inline int rsa_verify(struct rsa_ctx *ctx,
							 struct rsa_pkt *pkt)
{
	pkt->ctx = ctx;
	return ctx->ops.verify(ctx, pkt);
}

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_CRYPTO_RSA_H_ */
