/*
 * Copyright (c) 2016 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief ECDSA APIs
 *
 * This file contains the ECDSA Abstraction layer APIs.
 *
 * [Experimental] Users should note that the APIs can change
 * as a part of ongoing development.
 */

/**
 * @brief Crypto APIs
 * @defgroup crypto ECDSA
 * @{
 * @}
 */

/**
 * @brief Crypto ECDSA APIs
 * @defgroup crypto_ecdsa ECDSA
 * @ingroup crypto
 * @{
 */

#ifndef ZEPHYR_INCLUDE_CRYPTO_ECDSA_H_
#define ZEPHYR_INCLUDE_CRYPTO_ECDSA_H_

#include <device.h>
#include <errno.h>
#include <sys/util.h>
#include <sys/__assert.h>
#include "ecdsa_structs.h"

/* The API a ecdsa driver should implement */
__subsystem struct ecdsa_driver_api {
	int (*query_hw_caps)(const struct device *dev);

	/* Setup a ecdas session */
	int (*begin_session)(const struct device *dev, struct ecdsa_ctx *ctx,
						 struct ecdsa_key *key);

	/* Tear down an established session */
	int (*free_session)(const struct device *dev, struct ecdsa_ctx *ctx);
};

/**
 * @brief Query the ecdsa hardware capabilities
 *
 * This API is used by the app to query the capabilities supported by the
 * ecdsa device. Based on this the app can specify a subset of the supported
 * options to be honored for a session during ecdsa_begin_session().
 *
 * @param dev Pointer to the device structure for the driver instance.
 *
 * @return bitmask of supported options.
 */
static inline int ecdsa_query_hwcaps(const struct device *dev)
{
	struct ecdsa_driver_api *api;
	int tmp;

	api = (struct ecdsa_driver_api *)dev->api;

	tmp = api->query_hw_caps(dev);

	return tmp;
}

/**
 * @brief Setup a ecdsa session
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
static inline int ecdsa_begin_session(const struct device *dev,
									struct ecdsa_ctx *ctx, struct ecdsa_key *key)
{
	struct ecdsa_driver_api *api;

	api = (struct ecdsa_driver_api *)dev->api;
	ctx->device = dev;

	return api->begin_session(dev, ctx, key);
}

/**
 * @brief Cleanup a ecdsa session
 *
 * Clears the hardware and/or driver state of a previous session.
 *
 * @param  dev      Pointer to the device structure for the driver instance.
 * @param  ctx      Pointer to the ecdsa context structure of the session
 *			to be freed.
 *
 * @return 0 on success, negative errno code on fail.
 */
static inline int ecdsa_free_session(const struct device *dev,
								   struct ecdsa_ctx *ctx)
{
	struct ecdsa_driver_api *api;

	api = (struct ecdsa_driver_api *)dev->api;

	return api->free_session(dev, ctx);
}

/**
 * @brief Perform ECDSA sign.
 *
 * @param  ctx   Pointer to the ecdsa context of this op.
 * @param  pkt   Structure holding the input/output buffer pointers.
 *
 * @return 0 on success, negative errno code on fail.
 */
static inline int ecdsa_sign(struct ecdsa_ctx *ctx,
						   struct ecdsa_pkt *pkt)
{
	pkt->ctx = ctx;
	return ctx->ops.sign(ctx, pkt);
}

/**
 * @brief Perform ECDSA sign.
 *
 * @param  ctx   Pointer to the ecdsa context of this op.
 * @param  pkt   Structure holding the input/output buffer pointers.
 *
 * @return 0 on success, negative errno code on fail.
 */
static inline int ecdsa_verify(struct ecdsa_ctx *ctx,
							 struct ecdsa_pkt *pkt)
{
	pkt->ctx = ctx;
	return ctx->ops.verify(ctx, pkt);
}

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_CRYPTO_ECDSA_H_ */
