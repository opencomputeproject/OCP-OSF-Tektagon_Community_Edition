/*
 * Copyright (c) 2016 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Hash APIs
 *
 * This file contains the Hash Abstraction layer APIs.
 *
 * [Experimental] Users should note that the APIs can change
 * as a part of ongoing development.
 */

/**
 * @brief Crypto APIs
 * @defgroup crypto Hash
 * @{
 * @}
 */


/**
 * @brief Crypto Hash APIs
 * @defgroup crypto_hash Hash
 * @ingroup crypto
 * @{
 */

#ifndef ZEPHYR_INCLUDE_CRYPTO_HASH_H_
#define ZEPHYR_INCLUDE_CRYPTO_HASH_H_

#include <device.h>
#include <errno.h>
#include <sys/util.h>
#include <sys/__assert.h>
#include "hash_structs.h"

/* The API a hash driver should implement */
__subsystem struct hash_driver_api {
	int (*query_hw_caps)(const struct device *dev);

	/* Setup a hash session */
	int (*begin_session)(const struct device *dev, struct hash_ctx *ctx,
						 enum hash_algo algo);

	/* Tear down an established session */
	int (*free_session)(const struct device *dev, struct hash_ctx *ctx);
};

/**
 * @brief Query the hash hardware capabilities
 *
 * This API is used by the app to query the capabilities supported by the
 * hash device. Based on this the app can specify a subset of the supported
 * options to be honored for a session during hash_begin_session().
 *
 * @param dev Pointer to the device structure for the driver instance.
 *
 * @return bitmask of supported options.
 */
static inline int hash_query_hwcaps(const struct device *dev)
{
	struct hash_driver_api *api;
	int tmp;

	api = (struct hash_driver_api *) dev->api;

	tmp = api->query_hw_caps(dev);

	return tmp;
}

/**
 * @brief Setup a hash session
 *
 * Initializes one time parameters, like the session key, algorithm and cipher
 * mode which may remain constant for all operations in the session. The state
 * may be cached in hardware and/or driver data state variables.
 *
 * @param  dev      Pointer to the device structure for the driver instance.
 * @param  ctx      Pointer to the context structure.
 * @param  algo     The hash algorithm to be used in this session. e.g SHA512
 *
 * @return 0 on success, negative errno code on fail.
 */
static inline int hash_begin_session(const struct device *dev,
									 struct hash_ctx *ctx,
									 enum hash_algo algo)
{
	struct hash_driver_api *api;

	api = (struct hash_driver_api *) dev->api;
	ctx->device = dev;

	return api->begin_session(dev, ctx, algo);
}

/**
 * @brief Cleanup a hash session
 *
 * Clears the hardware and/or driver state of a previous session.
 *
 * @param  dev      Pointer to the device structure for the driver instance.
 * @param  ctx      Pointer to the hash context structure of the session
 *			to be freed.
 *
 * @return 0 on success, negative errno code on fail.
 */
static inline int hash_free_session(const struct device *dev,
									struct hash_ctx *ctx)
{
	struct hash_driver_api *api;

	api = (struct hash_driver_api *) dev->api;

	return api->free_session(dev, ctx);
}

/**
 * @brief Perform Hash update.
 *
 * @param  ctx   Pointer to the hash context of this op.
 * @param  pkt   Structure holding the input/output buffer pointers.
 *
 * @return 0 on success, negative errno code on fail.
 */
static inline int hash_update(struct hash_ctx *ctx,
							  struct hash_pkt *pkt)
{
	pkt->ctx = ctx;
	return ctx->ops.update_hndlr(ctx, pkt);
}

/**
 * @brief Perform Hash final.
 *
 * @param  ctx   Pointer to the hash context of this op.
 * @param  pkt   Structure holding the input/output buffer pointers.
 *
 * @return 0 on success, negative errno code on fail.
 */
static inline int hash_final(struct hash_ctx *ctx,
							  struct hash_pkt *pkt)
{
	pkt->ctx = ctx;
	return ctx->ops.final_hndlr(ctx, pkt);
}


/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_CRYPTO_HASH_H_ */
